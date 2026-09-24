#!/usr/bin/env node
import { chmod, lstat, mkdir, realpath, rename, rm, writeFile } from "node:fs/promises";
import { basename, dirname, isAbsolute, join, relative, resolve } from "node:path";
import { pathToFileURL } from "node:url";
import { loadRegistry, saveRegistry, type AliasRegistry } from "./aliases.js";
import { buildAwningEvidence } from "./awning-evidence-builder.js";
import { publishAwningEvidence } from "./awning-evidence-publication.js";
import { createPatch039LocalReadonlyClient } from "./awning-homey-api-adapter.js";
import { buildPrivateOperatorCandidates, createPrivateAwningMappingFromSelection } from "./awning-operator-selection.js";
import { loadPatch039PrivateConfig, type Patch039PrivateConfig } from "./awning-private-config.js";
import { collectPatch039ReadSurface, type Patch039ReadonlyHomeyClient } from "./awning-readonly-client.js";
import { MacOSKeychainCredentialProvider } from "./credential-provider.js";
import { CandidateError } from "./errors.js";
import { redact } from "./redaction.js";
import { loadPatch042PrivateSelection } from "./awning-private-selection.js";

export const PATCH042_LIVE_CAPTURE_CONTRACT = Object.freeze({
  connection: "strict_local",
  credential_provider: "macos-keychain",
  credential_kind: "personal_access_token",
  identity_gate_before_credential: true,
  candidate_discovery_reads: ["ManagerDevices.getDevices"],
  capture_reads: [
    "ManagerDevices.getDevices",
    "ManagerFlow.getFlows",
    "ManagerFlow.getFlowCardActions",
    "ManagerFlow.getAdvancedFlows",
  ],
  homey_mutation: "forbidden",
  flow_execution: "forbidden",
  advanced_flow_execution: "forbidden",
  firmware_change: "none",
});

export type Patch042Mode = "candidates" | "capture";

export interface Patch042RunOptions {
  mode: Patch042Mode;
  privateConfigPath: string;
  stateDir: string;
  generation: number;
  timeoutMs: number;
}

export interface Patch042CandidateDocument {
  schema_version: 1;
  purpose: "private_awning_operator_candidates";
  generation: number;
  selected_homey_id_sha256: string;
  candidate_count: number;
  candidates: Array<{
    device_alias: string;
    display_name: string | null;
    driver_alias: string | null;
    capability_aliases: string[];
  }>;
}

export type Patch042ClientFactory = (
  config: Patch039PrivateConfig,
  timeoutMs: number,
) => Promise<Patch039ReadonlyHomeyClient>;

function isInside(parent: string, candidate: string): boolean {
  const rel = relative(resolve(parent), resolve(candidate));
  return rel === "" || (!rel.startsWith("..") && !isAbsolute(rel));
}

function positiveInteger(value: string, label: string): number {
  const parsed = Number(value);
  if (!Number.isInteger(parsed) || parsed <= 0) {
    throw new CandidateError("CONFIGURATION", `Patch042 ${label} must be a positive integer`);
  }
  return parsed;
}

export function parsePatch042Args(argv: readonly string[]): Patch042RunOptions {
  const mode = argv[0];
  if (mode !== "candidates" && mode !== "capture") {
    throw new CandidateError("CONFIGURATION", "Patch042 mode must be candidates or capture");
  }
  let privateConfigPath = "";
  let stateDir = "";
  let generation = 0;
  let timeoutMs = 5000;

  for (let index = 1; index < argv.length; index += 1) {
    const arg = argv[index];
    const value = argv[index + 1];
    if (!value) throw new CandidateError("CONFIGURATION", `Patch042 missing value for ${arg}`);
    if (arg === "--private-config") privateConfigPath = resolve(value);
    else if (arg === "--state-dir") stateDir = resolve(value);
    else if (arg === "--generation") generation = positiveInteger(value, "generation");
    else if (arg === "--timeout-ms") timeoutMs = positiveInteger(value, "timeout-ms");
    else throw new CandidateError("CONFIGURATION", `Patch042 unknown argument ${arg}`);
    index += 1;
  }

  if (!privateConfigPath) throw new CandidateError("CONFIGURATION", "Patch042 --private-config is required");
  if (!stateDir) throw new CandidateError("CONFIGURATION", "Patch042 --state-dir is required");
  if (generation === 0) throw new CandidateError("CONFIGURATION", "Patch042 --generation is required");
  if (timeoutMs < 100) throw new CandidateError("CONFIGURATION", "Patch042 timeout-ms must be at least 100");
  return { mode, privateConfigPath, stateDir, generation, timeoutMs };
}

async function assertPrivateStateDirectory(stateDir: string, repositoryRoot: string): Promise<string> {
  if (!isAbsolute(stateDir)) {
    throw new CandidateError("CONFIGURATION", "Patch042 state directory path must be absolute");
  }
  const canonicalRepository = await realpath(repositoryRoot);
  const canonicalParent = await realpath(dirname(resolve(stateDir)));
  const candidate = join(canonicalParent, basename(stateDir));
  if (isInside(canonicalRepository, candidate)) {
    throw new CandidateError("CONFIGURATION", "Patch042 state directory must remain outside the repository");
  }

  try {
    const existing = await lstat(candidate);
    if (!existing.isDirectory() || existing.isSymbolicLink()) {
      throw new CandidateError("CONFIGURATION", "Patch042 state directory must be a real directory");
    }
  } catch (error) {
    const code = (error as NodeJS.ErrnoException).code;
    if (code !== "ENOENT") throw error;
    await mkdir(candidate, { mode: 0o700 });
  }

  const stat = await lstat(candidate);
  if (!stat.isDirectory() || stat.isSymbolicLink()) {
    throw new CandidateError("CONFIGURATION", "Patch042 state directory must be a real directory");
  }
  if ((stat.mode & 0o077) !== 0) {
    throw new CandidateError("CONFIGURATION", "Patch042 state directory permissions must not allow group or other access");
  }
  const canonicalState = await realpath(candidate);
  if (isInside(canonicalRepository, canonicalState)) {
    throw new CandidateError("CONFIGURATION", "Patch042 canonical state directory must remain outside the repository");
  }
  return canonicalState;
}

async function assertPrivateExistingFile(path: string, label: string): Promise<void> {
  try {
    const stat = await lstat(path);
    if (!stat.isFile() || stat.isSymbolicLink()) {
      throw new CandidateError("CONFIGURATION", `Patch042 ${label} must be a regular non-symlink file`);
    }
    if ((stat.mode & 0o077) !== 0) {
      throw new CandidateError("CONFIGURATION", `Patch042 ${label} permissions must not allow group or other access`);
    }
  } catch (error) {
    const code = (error as NodeJS.ErrnoException).code;
    if (code !== "ENOENT") throw error;
  }
}

async function loadPrivateAliasRegistry(path: string): Promise<AliasRegistry> {
  await assertPrivateExistingFile(path, "alias registry");
  return loadRegistry(path);
}

async function savePrivateAliasRegistry(path: string, registry: AliasRegistry): Promise<void> {
  await assertPrivateExistingFile(path, "alias registry");
  await saveRegistry(path, registry);
  await chmod(path, 0o600);
  await assertPrivateExistingFile(path, "alias registry");
}

async function writePrivateJson(path: string, value: unknown): Promise<void> {
  await assertPrivateExistingFile(path, "private output file");
  const tmp = `${path}.tmp-${process.pid}-${Date.now()}`;
  try {
    await writeFile(tmp, `${JSON.stringify(value, null, 2)}\n`, { mode: 0o600, flag: "wx" });
    await rename(tmp, path);
    await chmod(path, 0o600);
  } finally {
    await rm(tmp, { force: true });
  }
}

async function defaultClientFactory(
  config: Patch039PrivateConfig,
  timeoutMs: number,
): Promise<Patch039ReadonlyHomeyClient> {
  const provider = new MacOSKeychainCredentialProvider(config.keychain_service, config.keychain_account);
  return createPatch039LocalReadonlyClient({
    configuredAddress: config.homey_address,
    expectedHomeyDigest: config.expected_homey_id_sha256,
    getPersonalAccessToken: async () => (await provider.load()).token,
    timeoutMs,
  });
}

function assertCandidateReadBoundary(client: Patch039ReadonlyHomeyClient): void {
  const entries = client.ledger.entries();
  if (entries.length !== 1 || entries[0]?.operation !== "devices.read" || entries[0]?.result !== "success") {
    throw new CandidateError("AUTHORIZATION", "Patch042 candidate discovery exceeded the devices-only read boundary");
  }
  const session = client.sessionEvidence();
  if (!session.authentication_succeeded || !session.selected_homey_verified || !session.collection_attempted) {
    throw new CandidateError("AUTHENTICATION", "Patch042 candidate discovery did not establish authenticated selected-Homey evidence");
  }
}

export async function runPatch042Candidates(input: {
  repositoryRoot: string;
  privateConfigPath: string;
  stateDir: string;
  generation: number;
  timeoutMs: number;
  clientFactory?: Patch042ClientFactory;
}): Promise<Patch042CandidateDocument> {
  const config = await loadPatch039PrivateConfig(input.privateConfigPath, input.repositoryRoot);
  const privateState = await assertPrivateStateDirectory(input.stateDir, input.repositoryRoot);
  const aliasPath = join(privateState, "awning_alias_registry.json");
  const candidatesPath = join(privateState, "awning_candidates.json");
  const selectionTemplatePath = join(privateState, "awning_selection.template.json");
  const registry = await loadPrivateAliasRegistry(aliasPath);
  const client = await (input.clientFactory ?? defaultClientFactory)(config, input.timeoutMs);
  const devices = await client.getDevices();
  assertCandidateReadBoundary(client);
  const built = buildPrivateOperatorCandidates(devices, registry);

  const document: Patch042CandidateDocument = {
    schema_version: 1,
    purpose: "private_awning_operator_candidates",
    generation: input.generation,
    selected_homey_id_sha256: config.expected_homey_id_sha256,
    candidate_count: built.candidates.length,
    candidates: built.candidates,
  };
  const selectionTemplate = {
    schema_version: 1,
    purpose: "read_only_awning_operator_selection",
    generation: input.generation,
    selected_homey_id_sha256: config.expected_homey_id_sha256,
    selections: {
      awning_1: null,
      awning_2: null,
      awning_3: null,
    },
  };

  await writePrivateJson(candidatesPath, document);
  await writePrivateJson(selectionTemplatePath, selectionTemplate);
  await savePrivateAliasRegistry(aliasPath, registry);
  return document;
}

export async function runPatch042Capture(input: {
  repositoryRoot: string;
  privateConfigPath: string;
  stateDir: string;
  generation: number;
  timeoutMs: number;
  clientFactory?: Patch042ClientFactory;
}): Promise<Record<string, Record<string, string>>> {
  const config = await loadPatch039PrivateConfig(input.privateConfigPath, input.repositoryRoot);
  const privateState = await assertPrivateStateDirectory(input.stateDir, input.repositoryRoot);
  const aliasPath = join(privateState, "awning_alias_registry.json");
  const selectionPath = join(privateState, "awning_selection.json");
  const registry = await loadPrivateAliasRegistry(aliasPath);
  const selection = await loadPatch042PrivateSelection(
    selectionPath,
    input.repositoryRoot,
    config.expected_homey_id_sha256,
    input.generation,
  );

  const client = await (input.clientFactory ?? defaultClientFactory)(config, input.timeoutMs);
  const raw = await collectPatch039ReadSurface(client);
  const privateCandidates = buildPrivateOperatorCandidates(raw.devices, registry);
  const mapping = createPrivateAwningMappingFromSelection({
    selections: selection.selections,
    privateCorrelation: privateCandidates.privateCorrelation,
    selectedHomeyDigest: config.expected_homey_id_sha256,
    generation: input.generation,
  });
  const built = buildAwningEvidence({
    raw,
    mapping,
    expectedHomeyDigest: config.expected_homey_id_sha256,
    registry,
    client,
    generation: input.generation,
  });
  await publishAwningEvidence(privateState, input.repositoryRoot, built.evidence, built.rawIds);
  await savePrivateAliasRegistry(aliasPath, registry);

  const summary: Record<string, Record<string, string>> = {};
  for (const [role, actions] of Object.entries(built.evidence.actions)) {
    summary[role] = Object.fromEntries(
      Object.entries(actions).map(([action, evidence]) => [action, evidence.classification]),
    );
  }
  return summary;
}

export async function runPatch042(options: Patch042RunOptions, repositoryRoot: string): Promise<void> {
  if (options.mode === "candidates") {
    const result = await runPatch042Candidates({
      repositoryRoot,
      privateConfigPath: options.privateConfigPath,
      stateDir: options.stateDir,
      generation: options.generation,
      timeoutMs: options.timeoutMs,
    });
    console.log("PATCH042_CANDIDATE_DISCOVERY=PASS");
    console.log(`PATCH042_CANDIDATE_COUNT=${result.candidate_count}`);
    console.log("PATCH042_PRIVATE_CANDIDATES_FILE=awning_candidates.json");
    console.log("PATCH042_PRIVATE_SELECTION_TEMPLATE=awning_selection.template.json");
    console.log("PATCH042_REAL_HOMEY_READ_ONLY_CAPTURE=NOT_RUN");
  } else {
    const summary = await runPatch042Capture({
      repositoryRoot,
      privateConfigPath: options.privateConfigPath,
      stateDir: options.stateDir,
      generation: options.generation,
      timeoutMs: options.timeoutMs,
    });
    console.log("PATCH042_REAL_HOMEY_READ_ONLY_CAPTURE=PASS");
    console.log(`PATCH042_CLASSIFICATION_SUMMARY=${JSON.stringify(summary)}`);
    console.log("PATCH042_SANITIZED_EVIDENCE_FILE=awning_evidence.json");
  }
  console.log("PATCH042_HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED");
  console.log("PATCH042_FLOW_EXECUTION=NOT_RUN");
  console.log("PATCH042_ADVANCED_FLOW_EXECUTION=NOT_RUN");
  console.log("PATCH042_FIRMWARE_CHANGE=NONE");
}

async function main(): Promise<void> {
  const options = parsePatch042Args(process.argv.slice(2));
  const repositoryRoot = resolve(import.meta.dirname, "../../../..");
  await runPatch042(options, repositoryRoot);
}

const invoked = process.argv[1] ? pathToFileURL(resolve(process.argv[1])).href : "";
if (invoked === import.meta.url) {
  main().catch((error: unknown) => {
    const message = error instanceof Error ? error.message : String(error);
    console.error(`PATCH042_FAIL=${redact(message)}`);
    console.error("PATCH042_HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED");
    console.error("PATCH042_FLOW_EXECUTION=NOT_RUN");
    console.error("PATCH042_ADVANCED_FLOW_EXECUTION=NOT_RUN");
    process.exitCode = 1;
  });
}
