import assert from "node:assert/strict";
import { chmod, mkdir, mkdtemp, readFile, writeFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";
import test from "node:test";
import { Patch039CallLedger } from "../src/awning-call-ledger.js";
import type { Patch039ReadonlyHomeyClient } from "../src/awning-readonly-client.js";
import {
  inspectPatch042AwningCandidates,
  runPatch042AwningCapture,
  savePatch042AwningMapping,
} from "../src/awning-live-capture.js";
import { loadPrivateAwningMapping, savePrivateAwningMapping } from "../src/awning-private-mapping.js";
import type { Patch039SessionEvidence } from "../src/awning-model.js";

const rawIds = ["synthetic-awning-a", "synthetic-awning-b", "synthetic-awning-c"];
const expectedDigest = "b".repeat(64);

function devices() {
  return rawIds.map((id, index) => ({
    id,
    name: `Synthetic Awning ${index + 1}`,
    class: "sunshade",
    driverId: "synthetic-driver",
    capabilities: [],
    capabilitiesObj: {},
    available: true,
  }));
}

function fakeClient(mode: "devices-only" | "full"): Patch039ReadonlyHomeyClient {
  const ledger = new Patch039CallLedger();
  const session: Patch039SessionEvidence = {
    authentication_attempted: false,
    authentication_succeeded: false,
    selected_homey_verified: true,
    collection_attempted: false,
    publication_attempted: false,
  };
  const success = <T>(operation: Parameters<Patch039CallLedger["success"]>[0], value: T, count: number | null): T => {
    session.authentication_attempted = true;
    session.authentication_succeeded = true;
    session.collection_attempted = true;
    ledger.success(operation, count);
    return value;
  };
  return {
    ledger,
    async getDevices() {
      const value = devices();
      return success("devices.read", value, value.length);
    },
    async getFlows() {
      assert.equal(mode, "full");
      return success("flows.read", [], 0);
    },
    async getFlowCardActions() {
      assert.equal(mode, "full");
      return success("flow_card_actions.read", [], 0);
    },
    async getAdvancedFlows() {
      assert.equal(mode, "full");
      return success("advanced_flows.read", [], 0);
    },
    async getCapabilityValue() {
      throw new Error("Patch042 test did not authorize capability fallback");
    },
    sessionEvidence() {
      return { ...session };
    },
  };
}

async function privateFixture() {
  const repositoryRoot = await mkdtemp(join(tmpdir(), "patch042-repo-"));
  const privateRoot = await mkdtemp(join(tmpdir(), "patch042-private-"));
  await chmod(privateRoot, 0o700);
  const privateConfigPath = join(privateRoot, "config.json");
  await writeFile(privateConfigPath, JSON.stringify({
    schema_version: 1,
    connection: "local",
    credential_kind: "personal_access_token",
    credential_provider: "macos-keychain",
    keychain_service: "synthetic-service",
    keychain_account: "synthetic-account",
    homey_address: ["https:", "", "synthetic-homey.invalid"].join("/"),
    expected_homey_id_sha256: expectedDigest,
  }), { mode: 0o600 });
  await chmod(privateConfigPath, 0o600);
  return { repositoryRoot, privateRoot, privateConfigPath };
}

test("inspect publishes only sanitized candidates and no raw device IDs", async () => {
  const fixture = await privateFixture();
  const result = await inspectPatch042AwningCandidates({
    privateConfigPath: fixture.privateConfigPath,
    repositoryRoot: fixture.repositoryRoot,
    clientFactory: async () => fakeClient("devices-only"),
  });
  assert.equal(result.selected_homey_verified, true);
  assert.equal(result.authentication_succeeded, true);
  assert.equal(result.candidates.length, 3);
  const serialized = JSON.stringify(result);
  for (const rawId of rawIds) assert.equal(serialized.includes(rawId), false);
  for (const candidate of result.candidates) assert.match(candidate.device_alias, /^device_[0-9a-f]{12}$/);
});

test("map requires sanitized explicit selection and persists raw correlation only in private mapping", async () => {
  const fixture = await privateFixture();
  const inspection = await inspectPatch042AwningCandidates({
    privateConfigPath: fixture.privateConfigPath,
    repositoryRoot: fixture.repositoryRoot,
    clientFactory: async () => fakeClient("devices-only"),
  });
  const aliases = inspection.candidates.map((item) => item.device_alias);
  const mappingPath = join(fixture.privateRoot, "mapping.json");
  const result = await savePatch042AwningMapping({
    privateConfigPath: fixture.privateConfigPath,
    mappingPath,
    repositoryRoot: fixture.repositoryRoot,
    generation: 1,
    selections: { awning_1: aliases[0]!, awning_2: aliases[1]!, awning_3: aliases[2]! },
    clientFactory: async () => fakeClient("devices-only"),
  });
  const serializedResult = JSON.stringify(result);
  for (const rawId of rawIds) assert.equal(serializedResult.includes(rawId), false);
  const mapping = await loadPrivateAwningMapping(mappingPath, fixture.repositoryRoot);
  assert.deepEqual(Object.values(mapping.mappings).sort(), [...rawIds].sort());
});


test("capture rejects selected-Homey mapping mismatch before client construction", async () => {
  const fixture = await privateFixture();
  const mappingPath = join(fixture.privateRoot, "mapping-mismatch.json");
  await savePrivateAwningMapping(mappingPath, fixture.repositoryRoot, {
    schema_version: 1,
    purpose: "read_only_awning_evidence",
    generation: 1,
    selected_homey_id_sha256: "c".repeat(64),
    mappings: {
      awning_1: rawIds[0]!,
      awning_2: rawIds[1]!,
      awning_3: rawIds[2]!,
    },
  });
  const outputParent = join(fixture.privateRoot, "mismatch-output-parent");
  await mkdir(outputParent, { mode: 0o700 });
  let clientFactoryCalls = 0;
  await assert.rejects(runPatch042AwningCapture({
    privateConfigPath: fixture.privateConfigPath,
    mappingPath,
    outputDir: join(outputParent, "evidence"),
    repositoryRoot: fixture.repositoryRoot,
    clientFactory: async () => {
      clientFactoryCalls += 1;
      return fakeClient("full");
    },
  }), /selected-Homey mismatch/);
  assert.equal(clientFactoryCalls, 0);
});

test("capture executes only the locked base read surface and publishes sanitized evidence", async () => {
  const fixture = await privateFixture();
  const inspection = await inspectPatch042AwningCandidates({
    privateConfigPath: fixture.privateConfigPath,
    repositoryRoot: fixture.repositoryRoot,
    clientFactory: async () => fakeClient("devices-only"),
  });
  const aliases = inspection.candidates.map((item) => item.device_alias);
  const mappingPath = join(fixture.privateRoot, "mapping.json");
  await savePatch042AwningMapping({
    privateConfigPath: fixture.privateConfigPath,
    mappingPath,
    repositoryRoot: fixture.repositoryRoot,
    generation: 7,
    selections: { awning_1: aliases[0]!, awning_2: aliases[1]!, awning_3: aliases[2]! },
    clientFactory: async () => fakeClient("devices-only"),
  });

  const outputParent = join(fixture.privateRoot, "output-parent");
  await mkdir(outputParent, { mode: 0o700 });
  const outputDir = join(outputParent, "evidence");
  const result = await runPatch042AwningCapture({
    privateConfigPath: fixture.privateConfigPath,
    mappingPath,
    outputDir,
    repositoryRoot: fixture.repositoryRoot,
    clientFactory: async () => fakeClient("full"),
  });
  assert.equal(result.generation, 7);
  assert.equal(result.selected_homey_verified, true);
  assert.equal(result.collection_complete, true);
  assert.deepEqual(result.call_ledger.map((entry) => entry.operation), [
    "devices.read", "flows.read", "flow_card_actions.read", "advanced_flows.read",
  ]);
  const evidence = JSON.parse(await readFile(join(outputDir, "awning_evidence.json"), "utf8")) as Record<string, unknown>;
  const serialized = JSON.stringify(evidence);
  for (const rawId of rawIds) assert.equal(serialized.includes(rawId), false);
  assert.equal((evidence.session as Record<string, unknown>).publication_attempted, true);
});
