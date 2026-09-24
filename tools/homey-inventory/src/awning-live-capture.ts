import { MacOSKeychainCredentialProvider } from "./credential-provider.js";
import { CandidateError } from "./errors.js";
import { createPatch039LocalReadonlyClient } from "./awning-homey-api-adapter.js";
import { buildAwningEvidence } from "./awning-evidence-builder.js";
import { publishAwningEvidence } from "./awning-evidence-publication.js";
import type { Patch039ReadonlyHomeyClient } from "./awning-readonly-client.js";
import { collectPatch039ReadSurface } from "./awning-readonly-client.js";
import { loadPatch039PrivateConfig, type Patch039PrivateConfig } from "./awning-private-config.js";
import {
  assertMappingMatchesSelectedHomey,
  loadPrivateAwningMapping,
  savePrivateAwningMapping,
} from "./awning-private-mapping.js";
import {
  buildPrivateOperatorCandidates,
  createPrivateAwningMappingFromSelection,
  type PrivateOperatorCandidate,
} from "./awning-operator-selection.js";
import type { AwningRole, ClassifiedActionEvidence, AwningLedgerEntry } from "./awning-model.js";

export const PATCH042_LIVE_CAPTURE_CONTRACT = Object.freeze({
  purpose: "read_only_awning_evidence",
  transport: "patch040_strict_local_get_only",
  credential_provider: "macos-keychain",
  homey_mutation: false,
  flow_execution: false,
  advanced_flow_execution: false,
  write_methods: [] as const,
});

export type Patch042ClientFactory = (
  config: Patch039PrivateConfig,
) => Promise<Patch039ReadonlyHomeyClient>;

export interface Patch042InspectionResult {
  mode: "inspect";
  selected_homey_verified: true;
  authentication_succeeded: true;
  candidates: PrivateOperatorCandidate[];
}

export interface Patch042MappingResult {
  mode: "map";
  selected_homey_verified: true;
  authentication_succeeded: true;
  generation: number;
  selections: Record<AwningRole, string>;
}

export interface Patch042CaptureResult {
  mode: "capture";
  generation: number;
  selected_homey_verified: true;
  collection_complete: boolean;
  actions: Record<AwningRole, Record<"OPEN" | "STOP" | "CLOSE", ClassifiedActionEvidence>>;
  call_ledger: AwningLedgerEntry[];
  evidence_file: "awning_evidence.json";
}

function assertAuthenticatedSession(client: Patch039ReadonlyHomeyClient): void {
  const session = client.sessionEvidence();
  if (!session.selected_homey_verified) {
    throw new CandidateError("AUTHORIZATION", "Patch042 selected Homey identity was not verified");
  }
  if (!session.authentication_attempted || !session.authentication_succeeded) {
    throw new CandidateError("AUTHENTICATION", "Patch042 authenticated read did not succeed");
  }
}

export async function createPatch042LiveClient(
  config: Patch039PrivateConfig,
): Promise<Patch039ReadonlyHomeyClient> {
  const credentials = new MacOSKeychainCredentialProvider(
    config.keychain_service,
    config.keychain_account,
  );
  return createPatch039LocalReadonlyClient({
    configuredAddress: config.homey_address,
    expectedHomeyDigest: config.expected_homey_id_sha256,
    getPersonalAccessToken: async () => (await credentials.load()).token,
  });
}

async function loadClient(input: {
  privateConfigPath: string;
  repositoryRoot: string;
  clientFactory?: Patch042ClientFactory;
}): Promise<{ config: Patch039PrivateConfig; client: Patch039ReadonlyHomeyClient }> {
  const config = await loadPatch039PrivateConfig(input.privateConfigPath, input.repositoryRoot);
  const factory = input.clientFactory ?? createPatch042LiveClient;
  const client = await factory(config);
  return { config, client };
}

export async function inspectPatch042AwningCandidates(input: {
  privateConfigPath: string;
  repositoryRoot: string;
  clientFactory?: Patch042ClientFactory;
}): Promise<Patch042InspectionResult> {
  const { client } = await loadClient(input);
  const devices = await client.getDevices();
  assertAuthenticatedSession(client);
  const { candidates } = buildPrivateOperatorCandidates(devices, {});
  return {
    mode: "inspect",
    selected_homey_verified: true,
    authentication_succeeded: true,
    candidates,
  };
}

export async function savePatch042AwningMapping(input: {
  privateConfigPath: string;
  mappingPath: string;
  repositoryRoot: string;
  generation: number;
  selections: Record<AwningRole, string>;
  clientFactory?: Patch042ClientFactory;
}): Promise<Patch042MappingResult> {
  const { config, client } = await loadClient(input);
  const devices = await client.getDevices();
  assertAuthenticatedSession(client);
  const { privateCorrelation } = buildPrivateOperatorCandidates(devices, {});
  const mapping = createPrivateAwningMappingFromSelection({
    selections: input.selections,
    privateCorrelation,
    selectedHomeyDigest: config.expected_homey_id_sha256,
    generation: input.generation,
  });
  await savePrivateAwningMapping(input.mappingPath, input.repositoryRoot, mapping);
  return {
    mode: "map",
    selected_homey_verified: true,
    authentication_succeeded: true,
    generation: mapping.generation,
    selections: { ...input.selections },
  };
}

export async function runPatch042AwningCapture(input: {
  privateConfigPath: string;
  mappingPath: string;
  outputDir: string;
  repositoryRoot: string;
  clientFactory?: Patch042ClientFactory;
}): Promise<Patch042CaptureResult> {
  const config = await loadPatch039PrivateConfig(input.privateConfigPath, input.repositoryRoot);
  const mapping = await loadPrivateAwningMapping(input.mappingPath, input.repositoryRoot);
  assertMappingMatchesSelectedHomey(mapping, config.expected_homey_id_sha256);
  const factory = input.clientFactory ?? createPatch042LiveClient;
  const client = await factory(config);
  const raw = await collectPatch039ReadSurface(client);
  assertAuthenticatedSession(client);
  const { evidence, rawIds } = buildAwningEvidence({
    raw,
    mapping,
    expectedHomeyDigest: config.expected_homey_id_sha256,
    registry: {},
    client,
    generation: mapping.generation,
  });
  await publishAwningEvidence(input.outputDir, input.repositoryRoot, evidence, rawIds);
  return {
    mode: "capture",
    generation: evidence.generation,
    selected_homey_verified: true,
    collection_complete: evidence.collection_complete,
    actions: evidence.actions,
    call_ledger: evidence.call_ledger,
    evidence_file: "awning_evidence.json",
  };
}
