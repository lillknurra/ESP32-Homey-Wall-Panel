import { CandidateError } from "./errors.js";
import type { CandidateConnector, ReadonlyHomeyClient } from "./readonly-client.js";
import type { CandidateKind } from "./model.js";

/**
 * Patch 004 pins and loads the official homey-api package, but live connector
 * construction remains intentionally configuration-gated. This prevents tests
 * or validation from accidentally authenticating or issuing network requests.
 */
export class HomeyApiConnector implements CandidateConnector {
  constructor(public readonly kind: CandidateKind) {}

  async connect(_timeoutMs: number): Promise<ReadonlyHomeyClient> {
    await import("homey-api");
    throw new CandidateError(
      "CONFIGURATION",
      `Live ${this.kind} connector requires explicit private credential-provider configuration`,
    );
  }
}
