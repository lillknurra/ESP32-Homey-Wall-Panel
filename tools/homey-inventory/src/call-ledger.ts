import { redact } from "./redaction.js";

export const ALLOWED_READ_OPERATIONS = [
  "platform_metadata.read",
  "zones.read",
  "devices.read",
  "flows.read",
  "advanced_flows.read",
  "moods.read",
] as const;

export type ReadOperation = (typeof ALLOWED_READ_OPERATIONS)[number];

export class SanitizedCallLedger {
  readonly #entries: ReadOperation[] = [];

  record(operation: ReadOperation): void {
    if (!ALLOWED_READ_OPERATIONS.includes(operation)) {
      throw new Error(`Operation is not read-allowlisted: ${redact(String(operation))}`);
    }
    this.#entries.push(operation);
  }

  entries(): readonly ReadOperation[] {
    return [...this.#entries];
  }

  serialize(): string {
    return this.#entries.join("\n");
  }
}
