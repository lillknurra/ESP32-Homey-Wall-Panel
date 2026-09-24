import assert from "node:assert/strict";
import { chmod, mkdtemp, readFile, symlink, writeFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";
import test from "node:test";
import {
  PATCH043_REMOTE_ONLY_CONTRACT,
  PATCH044_NO_LOGIN_OAUTH_GATE,
  PATCH046_DIRECT_PINNED_HOMEY_API_CONTRACT,
  PATCH047_STORAGE_ADAPTER_INHERITANCE_CONTRACT,
  assertNoPatch043PatEnvironment,
  parsePatch043Args,
  runPatch043Candidates,
  runPatch043Homeys,
  listStoredOauthHomeysNoLogin,
  createDirectPinnedHomeyApiRemoteRuntime,
  createReadOnlyAthomCliOauthStore,
  resolveAthomCliSettingsPath,
  validatePatch043HomeySelection,
  type Patch043RemoteRuntime,
  type Patch046HomeyApiModule,
  type Patch047StorageAdapterConstructor,
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



class FakeStorageAdapter {
  async get(): Promise<Record<string, unknown>> {
    return {};
  }

  async set(): Promise<void> {}
}

test("Patch047 read-only OAuth store inherits the supplied AthomCloudAPI StorageAdapter base", async () => {
  const parent = await mkdtemp(join(tmpdir(), "patch047-store-base-"));
  const settingsPath = join(parent, "settings.json");
  await writeFile(settingsPath, JSON.stringify({
    homeyApi: { token: { access_token: "synthetic-access" } },
  }), { mode: 0o600 });
  await chmod(settingsPath, 0o600);

  const Base = FakeStorageAdapter as Patch047StorageAdapterConstructor;
  const store = createReadOnlyAthomCliOauthStore(Base, settingsPath);

  assert.equal(store instanceof FakeStorageAdapter, true);
  assert.equal(PATCH047_STORAGE_ADAPTER_INHERITANCE_CONTRACT.inheritance_required, true);
  assert.equal(PATCH047_STORAGE_ADAPTER_INHERITANCE_CONTRACT.storage_adapter_base, "AthomCloudAPI.StorageAdapter");
  assert.equal(PATCH047_STORAGE_ADAPTER_INHERITANCE_CONTRACT.oauth_store_write, "forbidden");
  await assert.rejects(
    store.set({ token: { access_token: "replacement" } }),
    /Patch047 refuses OAuth store writes/,
  );
});

test("Patch046 settings path follows HOMEY_HOME or the default athom-cli directory", () => {
  assert.equal(
    resolveAthomCliSettingsPath({ HOMEY_HOME: "/tmp/synthetic-homey-home" }, "/tmp/unused-home"),
    "/tmp/synthetic-homey-home/settings.json",
  );
  assert.equal(
    resolveAthomCliSettingsPath({}, "/tmp/synthetic-user"),
    "/tmp/synthetic-user/.athom-cli/settings.json",
  );
});

test("Patch046 read-only OAuth store returns only homeyApi and refuses every write", async () => {
  const parent = await mkdtemp(join(tmpdir(), "patch046-settings-"));
  const settingsPath = join(parent, "settings.json");
  const original = JSON.stringify({
    homeyApi: {
      token: {
        access_token: "synthetic-access",
        refresh_token: "synthetic-refresh",
      },
    },
    unrelated: { keep: true },
  });
  await writeFile(settingsPath, original, { mode: 0o600 });
  await chmod(settingsPath, 0o600);

  const store = createReadOnlyAthomCliOauthStore(FakeStorageAdapter, settingsPath);
  const loaded = await store.get();
  assert.deepEqual(Object.keys(loaded), ["token"]);
  assert.equal(JSON.stringify(loaded).includes("unrelated"), false);

  await assert.rejects(
    store.set({ token: { access_token: "replacement" } }),
    /refuses OAuth store writes/,
  );
  assert.equal(await readFile(settingsPath, "utf8"), original);
});

test("Patch046 read-only OAuth store rejects permissive files and symlinks", async () => {
  const parent = await mkdtemp(join(tmpdir(), "patch046-settings-mode-"));
  const settingsPath = join(parent, "settings.json");
  await writeFile(settingsPath, JSON.stringify({ homeyApi: { token: {} } }), { mode: 0o644 });
  await chmod(settingsPath, 0o644);

  await assert.rejects(
    createReadOnlyAthomCliOauthStore(FakeStorageAdapter, settingsPath).get(),
    /restrictive regular file/,
  );

  await chmod(settingsPath, 0o600);
  const linkPath = join(parent, "settings-link.json");
  await symlink(settingsPath, linkPath);
  await assert.rejects(
    createReadOnlyAthomCliOauthStore(linkPath).get(),
    /restrictive regular file/,
  );
});

test("Patch046 direct pinned runtime has no CLI package dependency, disables token refresh, and preserves remote-only strategy", async () => {
  const parent = await mkdtemp(join(tmpdir(), "patch046-runtime-"));
  const settingsPath = join(parent, "settings.json");
  await writeFile(settingsPath, JSON.stringify({
    homeyApi: { token: { access_token: "synthetic-access" } },
  }), { mode: 0o600 });
  await chmod(settingsPath, 0o600);

  const observed: Record<string, unknown> = {};
  class FakeCloud {
    static StorageAdapter = FakeStorageAdapter;

    constructor(input: { store: { get(): Promise<Record<string, unknown>> }; autoRefreshTokens: false }) {
      if (!(input.store instanceof FakeStorageAdapter)) {
        throw new Error("Invalid store. Must extend AthomCloudAPI/StorageAdapter.");
      }
      observed.autoRefreshTokens = input.autoRefreshTokens;
      observed.store = input.store;
      observed.storeIsStorageAdapter = input.store instanceof FakeStorageAdapter;
    }
    async isLoggedIn() {
      const store = observed.store as { get(): Promise<Record<string, unknown>> };
      observed.loaded = await store.get();
      return true;
    }
    async getAuthenticatedUser() {
      return {
        async getHomeys() {
          return [{
            id: "raw-homey-one",
            name: "Remote Homey",
            platform: "local",
            async authenticate(input: { strategy: string[] }) {
              observed.strategy = input.strategy;
              return { strategyId: input.strategy[0], devices: { async getDevices() { return {}; } } };
            },
          }];
        },
      };
    }
  }

  const module: Patch046HomeyApiModule = {
    AthomCloudAPI: FakeCloud as unknown as Patch046HomeyApiModule["AthomCloudAPI"],
    HomeyAPI: {
      PLATFORMS: { CLOUD: "cloud" },
      DISCOVERY_STRATEGIES: {
        CLOUD: "cloud",
        REMOTE_FORWARDED: "remoteForwarded",
      },
    },
  };

  const runtime = await createDirectPinnedHomeyApiRemoteRuntime({
    settingsPath,
    homeyApiModule: module,
  });
  const homeys = await runtime.getHomeysRemoteOnly();
  assert.equal(homeys.length, 1);
  const authenticated = await runtime.authenticateRemoteOnly(homeys[0]);
  assert.equal(observed.autoRefreshTokens, false);
  assert.equal(observed.storeIsStorageAdapter, true);
  assert.deepEqual(observed.strategy, ["remoteForwarded"]);
  assert.equal(PATCH046_DIRECT_PINNED_HOMEY_API_CONTRACT.cli_package_dependency, "none");
  assert.equal(PATCH046_DIRECT_PINNED_HOMEY_API_CONTRACT.oauth_store_write, "forbidden");
  assert.equal(PATCH046_DIRECT_PINNED_HOMEY_API_CONTRACT.auto_refresh_tokens, false);
  await runtime.dispose(authenticated.api);
});

test("Patch043 contract is Athom Internet-only and mutation-free", () => {
  assert.equal(PATCH043_REMOTE_ONLY_CONTRACT.homey_listing, "athom_cloud_stored_oauth_no_login");
  assert.equal(PATCH043_REMOTE_ONLY_CONTRACT.pro_strategy, "remoteForwarded");
  assert.equal(PATCH043_REMOTE_ONLY_CONTRACT.cloud_strategy, "cloud");
  assert.equal(PATCH043_REMOTE_ONLY_CONTRACT.local_discovery, "forbidden");
  assert.equal(PATCH043_REMOTE_ONLY_CONTRACT.local_pat, "forbidden");
  assert.equal(PATCH043_REMOTE_ONLY_CONTRACT.device_read, "ManagerDevices.getDevices");
  assert.equal(PATCH043_REMOTE_ONLY_CONTRACT.flow_read, "not_run");
  assert.equal(PATCH043_REMOTE_ONLY_CONTRACT.mutation, "forbidden");
});


test("Patch044 no-login OAuth gate refuses absent stored session without user fetch or login side effect", async () => {
  const counters = { isLoggedIn: 0, getAuthenticatedUser: 0, login: 0 };
  const cloud = {
    async isLoggedIn() {
      counters.isLoggedIn += 1;
      return false;
    },
    async getAuthenticatedUser() {
      counters.getAuthenticatedUser += 1;
      return {
        async getHomeys() {
          return [];
        },
      };
    },
    async login() {
      counters.login += 1;
    },
  };

  await assert.rejects(
    listStoredOauthHomeysNoLogin(cloud),
    /browser OAuth is required/,
  );
  assert.deepEqual(counters, { isLoggedIn: 1, getAuthenticatedUser: 0, login: 0 });
});

test("Patch044 no-login OAuth gate lists account Homeys from stored session without invoking login", async () => {
  const counters = { isLoggedIn: 0, getAuthenticatedUser: 0, getHomeys: 0, login: 0 };
  const cloud = {
    async isLoggedIn() {
      counters.isLoggedIn += 1;
      return true;
    },
    async getAuthenticatedUser() {
      counters.getAuthenticatedUser += 1;
      return {
        async getHomeys() {
          counters.getHomeys += 1;
          return [{ id: "remote-one" }];
        },
      };
    },
    async login() {
      counters.login += 1;
    },
  };

  const homeys = await listStoredOauthHomeysNoLogin(cloud);
  assert.equal(homeys.length, 1);
  assert.deepEqual(counters, { isLoggedIn: 1, getAuthenticatedUser: 1, getHomeys: 1, login: 0 });
  assert.equal(PATCH044_NO_LOGIN_OAUTH_GATE.browser_login_side_effect, "forbidden");
});

test("Patch044 no-login OAuth gate fails closed on authenticated-user errors without login fallback", async () => {
  const counters = { login: 0 };
  const cloud = {
    async isLoggedIn() {
      return true;
    },
    async getAuthenticatedUser(): Promise<{ getHomeys(): Promise<unknown[]> }> {
      throw new Error("synthetic invalid stored token");
    },
    async login() {
      counters.login += 1;
    },
  };
  await assert.rejects(
    listStoredOauthHomeysNoLogin(cloud),
    /could not authenticate without login/,
  );
  assert.equal(counters.login, 0);
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
