import type { RawInventory } from "./model.js";
import type { SanitizedCallLedger } from "./call-ledger.js";

export interface ReadonlyHomeyClient {
  collect(): Promise<RawInventory>;
  readonly ledger?: SanitizedCallLedger;
}

export interface CandidateConnector {
  readonly kind: import("./model.js").CandidateKind;
  connect(timeoutMs: number): Promise<ReadonlyHomeyClient>;
}

export const ALLOWED_READ_METHOD_NAMES = [
  "readPlatformMetadata",
  "readZones",
  "readDevices",
  "readFlows",
  "readAdvancedFlows",
  "readMoods",
] as const;

export const FORBIDDEN_MUTATION_NAMES = [
  "setCapabilityValue",
  "triggerFlow",
  "startFlow",
  "activateMood",
  "create",
  "update",
  "delete",
  "genericApiCall",
] as const;
