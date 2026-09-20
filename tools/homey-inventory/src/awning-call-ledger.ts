import type { FailureClass } from "./model.js";
import type { AwningLedgerEntry } from "./awning-model.js";

export const PATCH039_ALLOWED_READ_OPERATIONS = [
  "devices.read",
  "capability_value.read",
  "flows.read",
  "flow_card_actions.read",
  "advanced_flows.read",
] as const;

export type Patch039ReadOperation = (typeof PATCH039_ALLOWED_READ_OPERATIONS)[number];

export class Patch039CallLedger {
  #seq = 0;
  readonly #entries: AwningLedgerEntry[] = [];

  success(operation: Patch039ReadOperation, aggregateCount: number | null): void {
    this.#entries.push({
      seq: ++this.#seq,
      operation,
      result: "success",
      aggregate_count: aggregateCount,
      failure_class: null,
    });
  }

  failure(operation: Patch039ReadOperation, failureClass: FailureClass): void {
    this.#entries.push({
      seq: ++this.#seq,
      operation,
      result: "failure",
      aggregate_count: null,
      failure_class: failureClass,
    });
  }

  entries(): AwningLedgerEntry[] {
    return this.#entries.map((entry) => ({ ...entry }));
  }
}
