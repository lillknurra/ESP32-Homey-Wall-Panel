import assert from "node:assert/strict";
import { chmod, mkdtemp, readFile, writeFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";
import test from "node:test";
import {
  PATCH043_REMOTE_ONLY_CONTRACT,
  assertNoPatch043PatEnvironment,
  parsePatch043Args,
  runPatch043Candidates,
  runPatch043Homeys,
  validatePatch043HomeySelection,
  type Patch043RemoteRuntime,
} from "../src/awning-athom-remote-candidates.js";

function remoteRuntime(input: {
  homeys?: unknown[];
  devices?: unknown;
  strategy?: "remoteForwarded" | "cloud";
  counters?: Record<string, number>;
} = {}): Patch043RemoteRuntime {
  const counters = input.counters ?? {};
  const homeys = input.homeys ?? [{
    id: "raw-homey-one",
    name: "Remote Homey",
    platform: "local",
    state: "online",
    async authenticate() {},
  }];
  return {
    async getHomeysRemoteOnly() {
      counters.homeys = (counters.homeys ?? 0) + 1;
      return homeys;
    },
    async authenticateRemoteOnly(homey) {
      counters.auth = (counters.auth ?? 0) + 1;
      counters.lastHomey = (homey as { id?: string }).id === "raw-homey-one" ? 1 : -1;
      const strategyId = input.strategy ?? "remoteForwarded";
      return {
        strategyId,
        api: {
          strategyId,
          devices: {
            async getDevices() {
              counters.devices = (counters.devices ?? 0) + 1;
              return input.devices ?? {
                a: {
                  id: "raw-device-one",
                  name: "Awning One",
                  class: "blinds",
                  driverId: "raw-driver",
                  capabilities: ["windowcoverings_state"],
                },
              };
            },
          },
        },
      };
    },
    async dispose() {
      counters.dispose = (counters.dispose ?? 0) + 1;
    },
  };
}

async function fixture() {
  const repo = await mkdtemp(join(tmpdir(), "patch043-repo-"));
  const privateParent = await mkdtemp(join(tmpdir(), "patch043-private-"));
  await chmod(privateParent, 0o700);
  return {
    repo,
    stateDir: join(privateParent, "state"),
  };
}

test("Patch043 contract is Athom Internet-only and mutation-free", () => {
  assert.equal(PATCH043_REMOTE_ONLY_CONTRACT.homey_listing, "athom_cloud_account_local_false");
  assert.equal(PATCH043_REMOTE_ONLY_CONTRACT.pro_strategy, "remoteForwarded");
  assert.equal(PATCH043_REMOTE_ONLY_CONTRACT.cloud_strategy, "cloud");
  assert.equal(PATCH043_REMOTE_ONLY_CONTRACT.local_discovery, "forbidden");
  assert.equal(PATCH043_REMOTE_ONLY_CONTRACT.local_pat, "forbidden");
  assert.equal(PATCH043_REMOTE_ONLY_CONTRACT.device_read, "ManagerDevices.getDevices");
  assert.equal(PATCH043_REMOTE_ONLY_CONTRACT.flow_read, "not_run");
  assert.equal(PATCH043_REMOTE_ONLY_CONTRACT.mutation, "forbidden");
});

test("Patch043 rejects HOMEY_PAT and accepts no local address or token arguments", () => {
  assert.throws(
    () => assertNoPatch043PatEnvironment({ HOMEY_PAT: "synthetic-pat" }),
    /refuses HOMEY_PAT/,
  );
  assert.doesNotThrow(() => assertNoPatch043PatEnvironment({}));
  assert.throws(
    () => parsePatch043Args(["homeys", "--state-dir", "/tmp/x", "--generation", "1", "--address", "local"]),
    /unknown argument/,
  );
  assert.throws(
    () => parsePatch043Args(["candidates", "--state-dir", "/tmp/x", "--generation", "1", "--token", "x"]),
    /unknown argument/,
  );
});

test("Patch043 Homey selection accepts one canonical alias and fails closed on drift", () => {
  const valid = {
    schema_version: 1,
    purpose: "athom_remote_homey_selection",
    generation: 4,
    homey_alias: "homey_111111111111",
  };
  assert.equal(validatePatch043HomeySelection(valid, 4).homey_alias, "homey_111111111111");
  assert.throws(() => validatePatch043HomeySelection({ ...valid, generation: 5 }, 4), /generation mismatch/);
  assert.throws(() => validatePatch043HomeySelection({ ...valid, extra: true }, 4), /unknown or missing/);
  assert.throws(() => validatePatch043HomeySelection({ ...valid, homey_alias: "raw-id" }, 4), /alias is invalid/);
});

test("Homey listing uses only remote runtime and persists no raw Homey ID in sanitized document", async () => {
  const fx = await fixture();
  const counters: Record<string, number> = {};
  const doc = await runPatch043Homeys({
    repositoryRoot: fx.repo,
    stateDir: fx.stateDir,
    generation: 2,
    runtime: remoteRuntime({ counters }),
  });
  assert.equal(counters.homeys, 1);
  assert.equal(counters.auth ?? 0, 0);
  assert.equal(doc.homey_count, 1);
  assert.match(doc.homeys[0]!.homey_alias, /^homey_[0-9a-f]{12}$/);
  const publicDoc = await readFile(join(fx.stateDir, "athom_remote_homeys.json"), "utf8");
  assert.equal(publicDoc.includes("raw-homey-one"), false);
  const privateMap = await readFile(join(fx.stateDir, "athom_remote_homey_aliases.json"), "utf8");
  assert.equal(privateMap.includes("raw-homey-one"), true);
});

test("device candidates authenticate selected Homey remotely and perform exactly one devices read", async () => {
  const fx = await fixture();
  const counters: Record<string, number> = {};
  const runtime = remoteRuntime({ counters });
  const homeys = await runPatch043Homeys({
    repositoryRoot: fx.repo,
    stateDir: fx.stateDir,
    generation: 3,
    runtime,
  });
  const selection = {
    schema_version: 1,
    purpose: "athom_remote_homey_selection",
    generation: 3,
    homey_alias: homeys.homeys[0]!.homey_alias,
  };
  await writeFile(join(fx.stateDir, "athom_remote_homey_selection.json"), JSON.stringify(selection), { mode: 0o600 });
  await chmod(join(fx.stateDir, "athom_remote_homey_selection.json"), 0o600);

  const result = await runPatch043Candidates({
    repositoryRoot: fx.repo,
    stateDir: fx.stateDir,
    generation: 3,
    runtime,
  });
  assert.equal(result.strategyId, "remoteForwarded");
  assert.equal(counters.auth, 1);
  assert.equal(counters.devices, 1);
  assert.equal(counters.dispose, 1);
  const candidates = await readFile(join(fx.stateDir, "awning_candidates.json"), "utf8");
  assert.equal(candidates.includes("raw-homey-one"), false);
  assert.equal(candidates.includes("raw-device-one"), false);
  assert.equal(candidates.includes("raw-driver"), false);
  assert.match(candidates, /device_[0-9a-f]{12}/);
});

test("Homey Cloud strategy remains Internet-only and is preserved in sanitized candidate output", async () => {
  const fx = await fixture();
  const counters: Record<string, number> = {};
  const runtime = remoteRuntime({
    counters,
    strategy: "cloud",
    homeys: [{
      id: "raw-homey-one",
      name: "Cloud Homey",
      platform: "cloud",
      state: "online",
    }],
  });
  const homeys = await runPatch043Homeys({
    repositoryRoot: fx.repo,
    stateDir: fx.stateDir,
    generation: 5,
    runtime,
  });
  await writeFile(join(fx.stateDir, "athom_remote_homey_selection.json"), JSON.stringify({
    schema_version: 1,
    purpose: "athom_remote_homey_selection",
    generation: 5,
    homey_alias: homeys.homeys[0]!.homey_alias,
  }), { mode: 0o600 });
  await chmod(join(fx.stateDir, "athom_remote_homey_selection.json"), 0o600);
  const result = await runPatch043Candidates({
    repositoryRoot: fx.repo,
    stateDir: fx.stateDir,
    generation: 5,
    runtime,
  });
  assert.equal(result.strategyId, "cloud");
});
