import type { RawInventory } from "./model.js";

export interface ReadonlyHomeyClient {
  collect(): Promise<RawInventory>;
}

export interface CandidateConnector {
  readonly kind: import("./model.js").CandidateKind;
  connect(timeoutMs: number): Promise<ReadonlyHomeyClient>;
}

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
