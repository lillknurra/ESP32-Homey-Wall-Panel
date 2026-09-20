import type { FailureClass } from "./model.js";

export const AWNING_ROLES = ["awning_1", "awning_2", "awning_3"] as const;
export type AwningRole = (typeof AWNING_ROLES)[number];

export const AWNING_ACTIONS = ["OPEN", "STOP", "CLOSE"] as const;
export type AwningAction = (typeof AWNING_ACTIONS)[number];

export const AWNING_CANDIDATE_CLASSIFICATIONS = [
  "NO_CANDIDATE",
  "CANDIDATE_DEVICE_CAPABILITY",
  "CANDIDATE_FLOW",
  "CANDIDATE_ADVANCED_FLOW",
  "AMBIGUOUS",
  "INSUFFICIENT_EVIDENCE",
] as const;
export type AwningCandidateClassification = (typeof AWNING_CANDIDATE_CLASSIFICATIONS)[number];

export type CandidateSourceKind = "device_capability" | "flow" | "advanced_flow";

export interface CandidateObservation {
  role: AwningRole;
  action: AwningAction;
  source_kind: CandidateSourceKind;
  source_alias: string;
  structural_signature: string;
  blocking_reasons: string[];
  side_effect_device_count: number;
}

export interface ClassifiedActionEvidence {
  classification: AwningCandidateClassification;
  candidate_source_aliases: string[];
  blocking_reasons: string[];
}

export interface SanitizedCapabilityEvidence {
  capability_alias: string;
  value_type: "null" | "boolean" | "number" | "string" | "unknown";
  value: boolean | number | null;
  last_updated_present: boolean;
  observed_metadata_keys: string[];
}

export interface SanitizedDeviceEvidence {
  role: AwningRole;
  device_alias: string;
  available: boolean | null;
  class: string | null;
  driver_alias: string | null;
  capabilities: SanitizedCapabilityEvidence[];
}

export interface SanitizedFlowActionEvidence {
  card_alias: string;
  owner_alias: string | null;
  target_roles: AwningRole[];
  referenced_device_count: number;
  unknown_field_names: string[];
}

export interface SanitizedFlowEvidence {
  flow_alias: string;
  enabled: boolean | null;
  triggerable: boolean | null;
  action_count: number;
  actions: SanitizedFlowActionEvidence[];
}

export interface SanitizedAdvancedFlowCardEvidence {
  card_alias: string;
  owner_alias: string | null;
  type: "action" | "condition" | "trigger" | "unknown";
  target_roles: AwningRole[];
  referenced_device_count: number;
  unknown_field_names: string[];
}

export interface SanitizedAdvancedFlowEvidence {
  advanced_flow_alias: string;
  enabled: boolean | null;
  triggerable: boolean | null;
  card_count: number;
  cards: SanitizedAdvancedFlowCardEvidence[];
}

export interface AwningLedgerEntry {
  seq: number;
  operation:
    | "devices.read"
    | "capability_value.read"
    | "flows.read"
    | "flow_card_actions.read"
    | "advanced_flows.read";
  result: "success" | "failure";
  aggregate_count: number | null;
  failure_class: FailureClass | null;
}

export interface Patch039SessionEvidence {
  authentication_attempted: boolean;
  authentication_succeeded: boolean;
  selected_homey_verified: boolean;
  collection_attempted: boolean;
  publication_attempted: boolean;
}

export interface AwningEvidenceDocument {
  schema_version: "1.0.0";
  purpose: "read_only_awning_evidence";
  generation: number;
  selected_homey_verified: true;
  collection_complete: boolean;
  devices: SanitizedDeviceEvidence[];
  flows: SanitizedFlowEvidence[];
  advanced_flows: SanitizedAdvancedFlowEvidence[];
  actions: Record<AwningRole, Record<AwningAction, ClassifiedActionEvidence>>;
  call_ledger: AwningLedgerEntry[];
  session: Patch039SessionEvidence;
  privacy_report: {
    raw_ids_present: false;
    secrets_present: false;
    unknown_open_schema_values_passed_through: false;
  };
}

export interface RawAwningCollection {
  devices: unknown[];
  flows: unknown[];
  flowCardActions: unknown[];
  advancedFlows: unknown[];
}
