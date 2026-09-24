#!/usr/bin/env node
import { createHash } from "node:crypto";
import { execFileSync } from "node:child_process";
import { chmod, lstat, mkdir, readFile, realpath, rename, rm, writeFile } from "node:fs/promises";
import { createRequire } from "node:module";
import { dirname, isAbsolute, join, relative, resolve } from "node:path";
import { pathToFileURL } from "node:url";
import { loadRegistry, saveRegistry, type AliasRegistry } from "./aliases.js";
import { buildPrivateOperatorCandidates } from "./awning-operator-selection.js";
import { CandidateError } from "./errors.js";
import { assertSanitized, redact } from "./redaction.js";

export const PATCH043_REMOTE_ONLY_CONTRACT = Object.freeze({
  homey_listing: "athom_cloud_account_local_false",
  pro_strategy: "remoteForwarded",
  cloud_strategy: "cloud",
  local_discovery: "forbidden",
  local_pat: "forbidden",
  device_read: "ManagerDevices.getDevices",
  flow_read: "not_run",
  advanced_flow_read: "not_run",
  mutation: "forbidden",
  firmware_change: "none",
});

export type Patch043Mode = "homeys" | "candidates";

export interface Patch043RunOptions {
  mode: Patch043Mode;
  stateDir: string;
  generation: number;
}

export interface Patch043SanitizedHomey {
  homey_alias: string;
  display_name: string | null;
  platform: string | null;
  state: string | null;
}

export interface Patch043HomeyListDocument {
  schema_version: 1;
  purpose: "athom_remote_homey_candidates";
  generation: number;
  homey_count: number;
  homeys: Patch043SanitizedHomey[];
}

export interface Patch043HomeySelection {
  schema_version: 1;
  purpose: "athom_remote_homey_selection";
  generation: number;
  homey_alias: string;
}

export interface Patch043RemoteHomeyApi {
  devices?: {
    getDevices(): Promise<unknown>;
  };
  strategyId?: string;
  __strategyId?: string;
  __managers?: Record<string, { destroy?: () => void } | undefined>;
  __refreshMap?: Record<string, unknown>;
  disconnect?: () => Promise<unknown>;
  destroy?: () => void;
}

export interface Patch043RemoteRuntime {
  getHomeysRemoteOnly(): Promise<unknown[]>;
  authenticateRemoteOnly(homey: unknown): Promise<{ api: Patch043RemoteHomeyApi; strategyId: "remoteForwarded" | "cloud" }>;
  dispose(api: Patch043RemoteHomeyApi | null): Promise<void>;
}

interface RawHomeyRecord {
  id: string;
  name: string | null;
  platform: string | null;
  state: string | null;
  raw: unknown;
}

function isInside(parent: string, candidate: string): boolean {
  const rel = relative(resolve(parent), resolve(candidate));
  return rel === "" || (!rel.startsWith("..") && !isAbsolute(rel));
}

function positiveInteger(value: string, label: string): number {
  const parsed = Number(value);
  if (!Number.isInteger(parsed) || parsed <= 0) {
    throw new CandidateError("CONFIGURATION", `Patch043 ${label} must be a positive integer`);
  }
  return parsed;
}

function recordOf(value: unknown): Record<string, unknown> | null {
  return value && typeof value === "object" && !Array.isArray(value)
    ? value as Record<string, unknown>
    : null;
}

function collectionValues(value: unknown, label: string): unknown[] {
  if (Array.isArray(value)) return [...value];
  const record = recordOf(value);
  if (record) return Object.values(record);
  throw new CandidateError("SCHEMA_MISMATCH", `Patch043 ${label} response is not a collection`);
}

function normalizeHomey(raw: unknown): RawHomeyRecord | null {
  const record = recordOf(raw);
  if (!record) return null;
  const rawId = record.id ?? record._id;
  if (typeof rawId !== "string" || rawId.length === 0) return null;
  return {
    id: rawId,
    name: typeof record.name === "string" ? record.name : null,
    platform: typeof record.platform === "string" ? record.platform : null,
    state: typeof record.state === "string" ? record.state : null,
    raw,
  };
}

function homeyAlias(rawId: string): string {
  return `homey_${createHash("sha256").update(`homey:${rawId}`).digest("hex").slice(0, 12)}`;
}

export function assertNoPatch043PatEnvironment(environment: NodeJS.ProcessEnv = process.env): void {
  if (typeof environment.HOMEY_PAT === "string" && environment.HOMEY_PAT.trim().length > 0) {
    throw new CandidateError("AUTHORIZATION", "Patch043 refuses HOMEY_PAT and token-mode authentication");
  }
}

export function parsePatch043Args(argv: readonly string[]): Patch043RunOptions {
  const mode = argv[0];
  if (mode !== "homeys" && mode !== "candidates") {
    throw new CandidateError("CONFIGURATION", "Patch043 mode must be homeys or candidates");
  }

  let stateDir = "";
  let generation = 0;
  for (let index = 1; index < argv.length; index += 1) {
    const arg = argv[index];
    const value = argv[index + 1];
    if (!value) throw new CandidateError("CONFIGURATION", `Patch043 missing value for ${arg}`);
    if (arg === "--state-dir") stateDir = resolve(value);
    else if (arg === "--generation") generation = positiveInteger(value, "generation");
    else throw new CandidateError("CONFIGURATION", `Patch043 unknown argument ${arg}`);
    index += 1;
  }

  if (!stateDir) throw new CandidateError("CONFIGURATION", "Patch043 --state-dir is required");
  if (generation === 0) throw new CandidateError("CONFIGURATION", "Patch043 --generation is required");
  return { mode, stateDir, generation };
}

async function assertPrivateStateDir(path: string, repositoryRoot: string): Promise<string> {
  if (!isAbsolute(path)) {
    throw new CandidateError("CONFIGURATION", "Patch043 state directory must be absolute");
  }
  const repository = await realpath(repositoryRoot);
  const parent = await realpath(dirname(resolve(path)));
  const candidate = join(parent, path.split("/").pop()!);
  if (isInside(repository, candidate)) {
    throw new CandidateError("CONFIGURATION", "Patch043 state directory must remain outside the repository");
  }

  try {
    const existing = await lstat(candidate);
    if (!existing.isDirectory() || existing.isSymbolicLink()) {
      throw new CandidateError("CONFIGURATION", "Patch043 state directory must be a regular directory");
    }
  } catch (error) {
    if ((error as NodeJS.ErrnoException).code !== "ENOENT") throw error;
    await mkdir(candidate, { mode: 0o700 });
  }

  const stat = await lstat(candidate);
  if ((stat.mode & 0o077) !== 0) {
    throw new CandidateError("CONFIGURATION", "Patch043 state directory permissions are too broad");
  }
  return realpath(candidate);
}

async function writePrivateJson(path: string, value: unknown): Promise<void> {
  const tmp = `${path}.tmp-${process.pid}-${Date.now()}`;
  try {
    await writeFile(tmp, `${JSON.stringify(value, null, 2)}\n`, { mode: 0o600, flag: "wx" });
    await rename(tmp, path);
    await chmod(path, 0o600);
  } finally {
    await rm(tmp, { force: true });
  }
}

async function readPrivateJson(path: string, label: string): Promise<unknown> {
  const stat = await lstat(path).catch((error) => {
    throw new CandidateError("CONFIGURATION", `Patch043 ${label} is unavailable`, { cause: error });
  });
  if (!stat.isFile() || stat.isSymbolicLink() || (stat.mode & 0o077) !== 0) {
    throw new CandidateError("CONFIGURATION", `Patch043 ${label} must be a 0600-style regular file`);
  }
  try {
    return JSON.parse(await readFile(path, "utf8"));
  } catch (error) {
    throw new CandidateError("CONFIGURATION", `Patch043 ${label} is invalid JSON`, { cause: error });
  }
}

export function validatePatch043HomeySelection(
  value: unknown,
  generation: number,
): Patch043HomeySelection {
  const record = recordOf(value);
  if (!record) throw new CandidateError("CONFIGURATION", "Patch043 Homey selection must be an object");
  const expectedKeys = ["generation", "homey_alias", "purpose", "schema_version"].sort();
  if (JSON.stringify(Object.keys(record).sort()) !== JSON.stringify(expectedKeys)) {
    throw new CandidateError("CONFIGURATION", "Patch043 Homey selection contains unknown or missing fields");
  }
  if (record.schema_version !== 1 || record.purpose !== "athom_remote_homey_selection") {
    throw new CandidateError("CONFIGURATION", "Patch043 Homey selection schema or purpose mismatch");
  }
  if (record.generation !== generation) {
    throw new CandidateError("CONFIGURATION", "Patch043 Homey selection generation mismatch");
  }
  if (typeof record.homey_alias !== "string" || !/^homey_[0-9a-f]{12}$/.test(record.homey_alias)) {
    throw new CandidateError("CONFIGURATION", "Patch043 Homey selection alias is invalid");
  }
  return {
    schema_version: 1,
    purpose: "athom_remote_homey_selection",
    generation,
    homey_alias: record.homey_alias,
  };
}

function defaultHomeyCliRoot(): string {
  const override = process.env.PATCH043_HOMEY_CLI_ROOT?.trim();
  if (override) return resolve(override);
  let globalRoot = "";
  try {
    globalRoot = execFileSync("npm", ["root", "-g"], {
      encoding: "utf8",
      stdio: ["ignore", "pipe", "ignore"],
    }).trim();
  } catch (error) {
    throw new CandidateError("CONFIGURATION", "Patch043 could not locate the global npm package root", { cause: error });
  }
  if (!globalRoot) throw new CandidateError("CONFIGURATION", "Patch043 global npm package root is empty");
  return join(globalRoot, "homey");
}

export async function createOfficialHomeyCliRemoteRuntime(
  cliRoot = defaultHomeyCliRoot(),
): Promise<Patch043RemoteRuntime> {
  assertNoPatch043PatEnvironment();
  const packageJsonPath = join(cliRoot, "package.json");
  let packageJson: Record<string, unknown>;
  try {
    packageJson = JSON.parse(await readFile(packageJsonPath, "utf8")) as Record<string, unknown>;
  } catch (error) {
    throw new CandidateError("CONFIGURATION", "Patch043 official Homey CLI package is unavailable", { cause: error });
  }
  if (packageJson.name !== "homey") {
    throw new CandidateError("CONFIGURATION", "Patch043 resolved package is not the official Homey CLI package");
  }

  const requireFromCli = createRequire(packageJsonPath);
  const AthomApi = requireFromCli("./services/AthomApi.js") as {
    discoveryStrategies?: string[];
    getHomeys(input?: { cache?: boolean; local?: boolean }): Promise<unknown[]>;
  };
  const homeyApiModule = requireFromCli("homey-api") as {
    HomeyAPI: {
      PLATFORMS: { CLOUD: string };
      DISCOVERY_STRATEGIES: {
        CLOUD: string;
        REMOTE_FORWARDED: string;
      };
    };
  };
  const HomeyAPI = homeyApiModule.HomeyAPI;
  if (!HomeyAPI?.DISCOVERY_STRATEGIES?.REMOTE_FORWARDED || !HomeyAPI?.DISCOVERY_STRATEGIES?.CLOUD) {
    throw new CandidateError("API_INCOMPATIBILITY", "Patch043 required remote discovery strategies are unavailable");
  }

  return {
    async getHomeysRemoteOnly(): Promise<unknown[]> {
      AthomApi.discoveryStrategies = [HomeyAPI.DISCOVERY_STRATEGIES.REMOTE_FORWARDED];
      const homeys = await AthomApi.getHomeys({ cache: false, local: false });
      if (!Array.isArray(homeys)) {
        throw new CandidateError("SCHEMA_MISMATCH", "Patch043 Athom Homey list is not an array");
      }
      return homeys;
    },

    async authenticateRemoteOnly(homey: unknown) {
      const record = recordOf(homey);
      if (!record || typeof record.authenticate !== "function") {
        throw new CandidateError("API_INCOMPATIBILITY", "Patch043 Homey authentication surface is unavailable");
      }
      const requestedStrategy = record.platform === HomeyAPI.PLATFORMS.CLOUD
        ? HomeyAPI.DISCOVERY_STRATEGIES.CLOUD
        : HomeyAPI.DISCOVERY_STRATEGIES.REMOTE_FORWARDED;
      if (requestedStrategy !== "cloud" && requestedStrategy !== "remoteForwarded") {
        throw new CandidateError("AUTHORIZATION", "Patch043 resolved a non-remote discovery strategy");
      }
      AthomApi.discoveryStrategies = [requestedStrategy];
      const api = await (record.authenticate as (input: { strategy: string[] }) => Promise<Patch043RemoteHomeyApi>)({
        strategy: [requestedStrategy],
      });
      const resolved = api.strategyId ?? api.__strategyId;
      if (resolved !== undefined && resolved !== requestedStrategy) {
        await disposeApi(api);
        throw new CandidateError("AUTHORIZATION", "Patch043 Homey API resolved a strategy outside the requested remote-only strategy");
      }
      return {
        api,
        strategyId: requestedStrategy as "remoteForwarded" | "cloud",
      };
    },

    dispose: disposeApi,
  };
}

async function disposeApi(api: Patch043RemoteHomeyApi | null): Promise<void> {
  if (!api) return;
  for (const manager of Object.values(api.__managers ?? {})) {
    if (typeof manager?.destroy === "function") manager.destroy();
  }
  for (const [key, value] of Object.entries(api.__refreshMap ?? {})) {
    if (key.endsWith("timeout")) {
      clearTimeout(value as NodeJS.Timeout);
      delete api.__refreshMap![key];
    }
  }
  if (typeof api.disconnect === "function") await api.disconnect().catch(() => {});
  if (typeof api.destroy === "function") api.destroy();
}

function sanitizeHomeys(rawHomeys: readonly unknown[]): {
  documentHomeys: Patch043SanitizedHomey[];
  privateAliases: Record<string, string>;
  rawIds: string[];
} {
  const documentHomeys: Patch043SanitizedHomey[] = [];
  const privateAliases: Record<string, string> = {};
  const rawIds: string[] = [];
  for (const raw of rawHomeys) {
    const homey = normalizeHomey(raw);
    if (!homey) continue;
    const alias = homeyAlias(homey.id);
    if (privateAliases[alias] && privateAliases[alias] !== homey.id) {
      throw new CandidateError("SCHEMA_MISMATCH", "Patch043 Homey alias collision");
    }
    privateAliases[alias] = homey.id;
    rawIds.push(homey.id);
    documentHomeys.push({
      homey_alias: alias,
      display_name: homey.name,
      platform: homey.platform,
      state: homey.state,
    });
  }
  documentHomeys.sort((a, b) => a.homey_alias.localeCompare(b.homey_alias));
  return { documentHomeys, privateAliases, rawIds };
}

export async function runPatch043Homeys(input: {
  repositoryRoot: string;
  stateDir: string;
  generation: number;
  runtime?: Patch043RemoteRuntime;
}): Promise<Patch043HomeyListDocument> {
  assertNoPatch043PatEnvironment();
  const stateDir = await assertPrivateStateDir(input.stateDir, input.repositoryRoot);
  const runtime = input.runtime ?? await createOfficialHomeyCliRemoteRuntime();
  const rawHomeys = await runtime.getHomeysRemoteOnly();
  const sanitized = sanitizeHomeys(rawHomeys);
  const document: Patch043HomeyListDocument = {
    schema_version: 1,
    purpose: "athom_remote_homey_candidates",
    generation: input.generation,
    homey_count: sanitized.documentHomeys.length,
    homeys: sanitized.documentHomeys,
  };

  const serialized = JSON.stringify(document);
  assertSanitized(serialized, sanitized.rawIds);
  await writePrivateJson(join(stateDir, "athom_remote_homeys.json"), document);
  await writePrivateJson(join(stateDir, "athom_remote_homey_aliases.json"), {
    schema_version: 1,
    generation: input.generation,
    aliases: sanitized.privateAliases,
  });
  await writePrivateJson(join(stateDir, "athom_remote_homey_selection.template.json"), {
    schema_version: 1,
    purpose: "athom_remote_homey_selection",
    generation: input.generation,
    homey_alias: null,
  });
  return document;
}

export async function runPatch043Candidates(input: {
  repositoryRoot: string;
  stateDir: string;
  generation: number;
  runtime?: Patch043RemoteRuntime;
}): Promise<{ candidateCount: number; strategyId: "remoteForwarded" | "cloud" }> {
  assertNoPatch043PatEnvironment();
  const stateDir = await assertPrivateStateDir(input.stateDir, input.repositoryRoot);
  const selection = validatePatch043HomeySelection(
    await readPrivateJson(join(stateDir, "athom_remote_homey_selection.json"), "Homey selection"),
    input.generation,
  );
  const aliasDocument = recordOf(
    await readPrivateJson(join(stateDir, "athom_remote_homey_aliases.json"), "Homey alias map"),
  );
  const aliases = aliasDocument && recordOf(aliasDocument.aliases);
  if (!aliasDocument || aliasDocument.schema_version !== 1 || aliasDocument.generation !== input.generation || !aliases) {
    throw new CandidateError("CONFIGURATION", "Patch043 Homey alias map schema mismatch");
  }
  const rawHomeyId = aliases[selection.homey_alias];
  if (typeof rawHomeyId !== "string" || rawHomeyId.length === 0) {
    throw new CandidateError("CONFIGURATION", "Patch043 selected Homey alias is unknown");
  }

  const runtime = input.runtime ?? await createOfficialHomeyCliRemoteRuntime();
  const remoteHomeys = await runtime.getHomeysRemoteOnly();
  const selected = remoteHomeys
    .map((raw) => normalizeHomey(raw))
    .find((homey) => homey?.id === rawHomeyId);
  if (!selected) {
    throw new CandidateError("AUTHORIZATION", "Patch043 selected Homey is absent from the fresh Athom account inventory");
  }

  let api: Patch043RemoteHomeyApi | null = null;
  try {
    const authenticated = await runtime.authenticateRemoteOnly(selected.raw);
    api = authenticated.api;
    if (!api.devices || typeof api.devices.getDevices !== "function") {
      throw new CandidateError("API_INCOMPATIBILITY", "Patch043 ManagerDevices.getDevices is unavailable");
    }
    const rawDevicesValue = await api.devices.getDevices();
    const rawDevices = collectionValues(rawDevicesValue, "devices");
    const registryPath = join(stateDir, "awning_alias_registry.json");
    const registry: AliasRegistry = await loadRegistry(registryPath);
    const built = buildPrivateOperatorCandidates(rawDevices, registry);
    const document = {
      schema_version: 1,
      purpose: "athom_remote_awning_device_candidates",
      generation: input.generation,
      selected_homey_alias: selection.homey_alias,
      strategy: authenticated.strategyId,
      candidate_count: built.candidates.length,
      candidates: built.candidates,
    };
    const rawDeviceIds = [...built.privateCorrelation.values()];
    assertSanitized(JSON.stringify(document), [rawHomeyId, ...rawDeviceIds]);
    await writePrivateJson(join(stateDir, "awning_candidates.json"), document);
    await saveRegistry(registryPath, registry);
    await chmod(registryPath, 0o600);
    return {
      candidateCount: built.candidates.length,
      strategyId: authenticated.strategyId,
    };
  } finally {
    await runtime.dispose(api);
  }
}

export async function runPatch043(options: Patch043RunOptions, repositoryRoot: string): Promise<void> {
  if (options.mode === "homeys") {
    const document = await runPatch043Homeys({
      repositoryRoot,
      stateDir: options.stateDir,
      generation: options.generation,
    });
    console.log("PATCH043_ATHOM_HOMEY_LIST=PASS");
    console.log(`PATCH043_HOMEY_COUNT=${document.homey_count}`);
    console.log("PATCH043_LOCAL_DISCOVERY=NOT_RUN_AND_FORBIDDEN");
    console.log("PATCH043_HOMEY_DEVICE_READ=NOT_RUN");
  } else {
    const result = await runPatch043Candidates({
      repositoryRoot,
      stateDir: options.stateDir,
      generation: options.generation,
    });
    console.log("PATCH043_REMOTE_DEVICE_CANDIDATES=PASS");
    console.log(`PATCH043_REMOTE_STRATEGY=${result.strategyId}`);
    console.log(`PATCH043_CANDIDATE_COUNT=${result.candidateCount}`);
    console.log("PATCH043_LOCAL_DISCOVERY=NOT_RUN_AND_FORBIDDEN");
    console.log("PATCH043_HOMEY_DEVICE_READ=PASS__GETDEVICES_ONLY");
  }
  console.log("PATCH043_HOMEY_PAT=NOT_USED_AND_FORBIDDEN");
  console.log("PATCH043_FLOW_READ=NOT_RUN");
  console.log("PATCH043_ADVANCED_FLOW_READ=NOT_RUN");
  console.log("PATCH043_HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED");
  console.log("PATCH043_FIRMWARE_CHANGE=NONE");
}

async function main(): Promise<void> {
  const options = parsePatch043Args(process.argv.slice(2));
  const repositoryRoot = resolve(import.meta.dirname, "../../../..");
  await runPatch043(options, repositoryRoot);
}

const invoked = process.argv[1] ? pathToFileURL(resolve(process.argv[1])).href : "";
if (invoked === import.meta.url) {
  main().catch((error: unknown) => {
    const message = error instanceof Error ? error.message : String(error);
    console.error(`PATCH043_FAIL=${redact(message)}`);
    console.error("PATCH043_LOCAL_DISCOVERY=NOT_RUN_AND_FORBIDDEN");
    console.error("PATCH043_HOMEY_PAT=NOT_USED_AND_FORBIDDEN");
    console.error("PATCH043_HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED");
    process.exitCode = 1;
  });
}
