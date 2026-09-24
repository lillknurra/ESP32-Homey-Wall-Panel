import assert from "node:assert/strict";
import { createHash } from "node:crypto";
import { chmod, mkdtemp, readFile, rename, writeFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";
import test from "node:test";
import {
  PATCH042_LIVE_CAPTURE_CONTRACT,
  parsePatch042Args,
  runPatch042Candidates,
  runPatch042Capture,
} from "../src/awning-live-capture.js";
import { wrapVerifiedPatch039HomeyApi } from "../src/awning-homey-api-adapter.js";

const rawHomeyId = "synthetic-patch042-homey";
const digest = createHash("sha256").update(rawHomeyId).digest("hex");
const generation = 4;

function rawDevice(id: string, name: string) {
  return {
    id,
    name,
    available: true,
    class: "blinds",
    driverId: "synthetic-patch042-driver",
    capabilities: ["command"],
    capabilitiesObj: {
      command: {
        type: "enum",
        setable: true,
        values: [
          { id: "raise", title: "Open" },
          { id: "halt", title: "Stop" },
          { id: "lower", title: "Close" },
        ],
        value: "halt",
      },
    },
  };
}

const deviceIds = ["raw-awning-one", "raw-awning-two", "raw-awning-three"];

function fakeRawApi(counters: Record<string, number>) {
  return {
    id: rawHomeyId,
    devices: {
      async getDevices() {
        counters.devices = (counters.devices ?? 0) + 1;
        return [
          rawDevice(deviceIds[0]!, "Awning One"),
          rawDevice(deviceIds[1]!, "Awning Two"),
          rawDevice(deviceIds[2]!, "Awning Three"),
        ];
      },
      async getCapabilityValue() {
        counters.capability = (counters.capability ?? 0) + 1;
        return null;
      },
    },
    flow: {
      async getFlows() {
        counters.flows = (counters.flows ?? 0) + 1;
        return [];
      },
      async getFlowCardActions() {
        counters.flowCards = (counters.flowCards ?? 0) + 1;
        return [];
      },
      async getAdvancedFlows() {
        counters.advancedFlows = (counters.advancedFlows ?? 0) + 1;
        return [];
      },
    },
  };
}

async function fixture() {
  const repo = await mkdtemp(join(tmpdir(), "patch042-live-repo-"));
  const privateParent = await mkdtemp(join(tmpdir(), "patch042-live-private-"));
  await chmod(privateParent, 0o700);
  const stateDir = join(privateParent, "state");
  const configPath = join(privateParent, "config.json");
  await writeFile(configPath, JSON.stringify({
    schema_version: 1,
    connection: "local",
    credential_kind: "personal_access_token",
    credential_provider: "macos-keychain",
    keychain_service: "synthetic-service",
    keychain_account: "synthetic-account",
    homey_address: "https://synthetic-homey.invalid",
    expected_homey_id_sha256: digest,
  }), { mode: 0o600 });
  await chmod(configPath, 0o600);
  return { repo, stateDir, configPath };
}

test("Patch042 contract is strict-local Keychain PAT and execution-free", () => {
  assert.equal(PATCH042_LIVE_CAPTURE_CONTRACT.connection, "strict_local");
  assert.equal(PATCH042_LIVE_CAPTURE_CONTRACT.credential_provider, "macos-keychain");
  assert.deepEqual(PATCH042_LIVE_CAPTURE_CONTRACT.candidate_discovery_reads, ["ManagerDevices.getDevices"]);
  assert.equal(PATCH042_LIVE_CAPTURE_CONTRACT.homey_mutation, "forbidden");
  assert.equal(PATCH042_LIVE_CAPTURE_CONTRACT.flow_execution, "forbidden");
  assert.equal(PATCH042_LIVE_CAPTURE_CONTRACT.advanced_flow_execution, "forbidden");
});

test("Patch042 argument parser requires explicit mode, private config, state dir and generation", () => {
  const parsed = parsePatch042Args([
    "candidates",
    "--private-config", "/tmp/private.json",
    "--state-dir", "/tmp/state",
    "--generation", "3",
  ]);
  assert.equal(parsed.mode, "candidates");
  assert.equal(parsed.generation, 3);
  assert.equal(parsed.timeoutMs, 5000);
  assert.throws(() => parsePatch042Args(["capture"]), /private-config/);
  assert.throws(() => parsePatch042Args(["unknown", "--private-config", "/tmp/x"]), /mode/);
});

test("candidate discovery performs exactly one devices read and writes only sanitized aliases to private candidate file", async () => {
  const fx = await fixture();
  const counters: Record<string, number> = {};
  const result = await runPatch042Candidates({
    repositoryRoot: fx.repo,
    privateConfigPath: fx.configPath,
    stateDir: fx.stateDir,
    generation,
    timeoutMs: 1000,
    clientFactory: async () => wrapVerifiedPatch039HomeyApi(fakeRawApi(counters), digest),
  });
  assert.equal(counters.devices, 1);
  assert.equal(counters.flows ?? 0, 0);
  assert.equal(counters.flowCards ?? 0, 0);
  assert.equal(counters.advancedFlows ?? 0, 0);
  assert.equal(counters.capability ?? 0, 0);
  assert.equal(result.candidate_count, 3);

  const serialized = await readFile(join(fx.stateDir, "awning_candidates.json"), "utf8");
  for (const id of deviceIds) assert.equal(serialized.includes(id), false);
  assert.equal(serialized.includes(rawHomeyId), false);
  assert.match(serialized, /device_[0-9a-f]{12}/);
  assert.equal((await readFile(join(fx.stateDir, "awning_alias_registry.json"), "utf8")).includes(deviceIds[0]!), true);
});

test("capture uses fresh private alias correlation, performs no capability fallback or execution, and publishes sanitized evidence", async () => {
  const fx = await fixture();
  const candidateCounters: Record<string, number> = {};
  const candidates = await runPatch042Candidates({
    repositoryRoot: fx.repo,
    privateConfigPath: fx.configPath,
    stateDir: fx.stateDir,
    generation,
    timeoutMs: 1000,
    clientFactory: async () => wrapVerifiedPatch039HomeyApi(fakeRawApi(candidateCounters), digest),
  });
  const byName = new Map(candidates.candidates.map((item) => [item.display_name, item.device_alias]));
  const selection = {
    schema_version: 1,
    purpose: "read_only_awning_operator_selection",
    generation,
    selected_homey_id_sha256: digest,
    selections: {
      awning_1: byName.get("Awning One"),
      awning_2: byName.get("Awning Two"),
      awning_3: byName.get("Awning Three"),
    },
  };
  const tmpSelection = join(fx.stateDir, "selection.tmp");
  await writeFile(tmpSelection, JSON.stringify(selection), { mode: 0o600 });
  await chmod(tmpSelection, 0o600);
  await rename(tmpSelection, join(fx.stateDir, "awning_selection.json"));

  const counters: Record<string, number> = {};
  const summary = await runPatch042Capture({
    repositoryRoot: fx.repo,
    privateConfigPath: fx.configPath,
    stateDir: fx.stateDir,
    generation,
    timeoutMs: 1000,
    clientFactory: async () => wrapVerifiedPatch039HomeyApi(fakeRawApi(counters), digest),
  });
  assert.deepEqual(counters, { devices: 1, flows: 1, flowCards: 1, advancedFlows: 1 });
  assert.equal(summary.awning_1?.OPEN, "CANDIDATE_DEVICE_CAPABILITY");
  assert.equal(summary.awning_2?.STOP, "CANDIDATE_DEVICE_CAPABILITY");
  assert.equal(summary.awning_3?.CLOSE, "CANDIDATE_DEVICE_CAPABILITY");

  const evidence = await readFile(join(fx.stateDir, "awning_evidence.json"), "utf8");
  for (const id of deviceIds) assert.equal(evidence.includes(id), false);
  assert.equal(evidence.includes(rawHomeyId), false);
  assert.equal(evidence.includes("raise"), false);
  assert.equal(evidence.includes("halt"), false);
  assert.equal(evidence.includes("lower"), false);
  assert.match(evidence, /"publication_attempted": true/);
});

test("capture refuses selection not present in the fresh device inventory before evidence publication", async () => {
  const fx = await fixture();
  const candidateCounters: Record<string, number> = {};
  const candidates = await runPatch042Candidates({
    repositoryRoot: fx.repo,
    privateConfigPath: fx.configPath,
    stateDir: fx.stateDir,
    generation,
    timeoutMs: 1000,
    clientFactory: async () => wrapVerifiedPatch039HomeyApi(fakeRawApi(candidateCounters), digest),
  });
  const aliases = candidates.candidates.map((item) => item.device_alias);
  const selection = {
    schema_version: 1,
    purpose: "read_only_awning_operator_selection",
    generation,
    selected_homey_id_sha256: digest,
    selections: {
      awning_1: aliases[0],
      awning_2: aliases[1],
      awning_3: "device_ffffffffffff",
    },
  };
  await writeFile(join(fx.stateDir, "awning_selection.json"), JSON.stringify(selection), { mode: 0o600 });
  await chmod(join(fx.stateDir, "awning_selection.json"), 0o600);

  await assert.rejects(
    runPatch042Capture({
      repositoryRoot: fx.repo,
      privateConfigPath: fx.configPath,
      stateDir: fx.stateDir,
      generation,
      timeoutMs: 1000,
      clientFactory: async () => wrapVerifiedPatch039HomeyApi(fakeRawApi({}), digest),
    }),
    /unknown sanitized candidate/,
  );
});
