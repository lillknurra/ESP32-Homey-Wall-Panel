import type { AliasRegistry } from "./aliases.js";
import { AWNING_ROLES, type AwningEvidenceDocument, type RawAwningCollection } from "./awning-model.js";
import type { Patch039ReadonlyHomeyClient } from "./awning-readonly-client.js";
import type { PrivateAwningMapping } from "./awning-private-mapping.js";
import { assertMappedDevicesFresh, assertMappingMatchesSelectedHomey } from "./awning-private-mapping.js";
import { normalizeAwningEvidenceSources } from "./awning-evidence-normalizer.js";
import { classifyAwningCandidates } from "./awning-candidate-classifier.js";


function assertCompleteBaseLedger(client: Patch039ReadonlyHomeyClient): void {
  const entries = client.ledger.entries();
  if (entries.some((entry) => entry.result !== "success")) {
    throw new Error("Patch039 failed read operation forbids evidence build");
  }
  for (const operation of ["devices.read", "flows.read", "flow_card_actions.read", "advanced_flows.read"] as const) {
    if (entries.filter((entry) => entry.operation === operation).length !== 1) {
      throw new Error("Patch039 base read surface must complete exactly once before evidence build");
    }
  }
}

function rawDeviceIds(raw: RawAwningCollection): string[] {
  const result: string[] = [];
  for (const value of raw.devices) {
    if (!value || typeof value !== "object" || Array.isArray(value)) continue;
    const record = value as Record<string, unknown>;
    const id = record.id ?? record._id;
    if (typeof id === "string" && id.length > 0) result.push(id);
  }
  return result;
}

export function buildAwningEvidence(input: {
  raw: RawAwningCollection;
  mapping: PrivateAwningMapping;
  expectedHomeyDigest: string;
  registry: AliasRegistry;
  client: Patch039ReadonlyHomeyClient;
  generation: number;
}): { evidence: AwningEvidenceDocument; rawIds: string[] } {
  assertMappingMatchesSelectedHomey(input.mapping, input.expectedHomeyDigest);
  assertMappedDevicesFresh(input.mapping, rawDeviceIds(input.raw));
  const normalized = normalizeAwningEvidenceSources(input.raw, input.mapping, input.registry);
  const classified = classifyAwningCandidates(normalized.observations, normalized.negativeEvidenceComplete);
  const session = input.client.sessionEvidence();
  assertCompleteBaseLedger(input.client);
  if (!session.selected_homey_verified) throw new Error("Patch039 selected Homey must be verified before evidence build");
  if (!session.authentication_succeeded) throw new Error("Patch039 authenticated read must succeed before evidence build");

  for (const role of AWNING_ROLES) {
    if (!normalized.devices.some((device) => device.role === role)) {
      throw new Error("Patch039 mapped device evidence is incomplete");
    }
  }

  return {
    rawIds: normalized.rawIds,
    evidence: {
      schema_version: "1.0.0",
      purpose: "read_only_awning_evidence",
      generation: input.generation,
      selected_homey_verified: true,
      collection_complete: normalized.negativeEvidenceComplete,
      devices: normalized.devices,
      flows: normalized.flows,
      advanced_flows: normalized.advancedFlows,
      actions: classified,
      call_ledger: input.client.ledger.entries(),
      session: { ...session },
      privacy_report: {
        raw_ids_present: false,
        secrets_present: false,
        unknown_open_schema_values_passed_through: false,
      },
    },
  };
}
