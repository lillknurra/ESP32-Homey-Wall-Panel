import type { CandidateKind } from "./model.js";
import {
  ALLOWED_READ_OPERATIONS,
  type ReadOperation,
} from "./call-ledger.js";
import { CandidateError } from "./errors.js";

const ALLOWED_CANDIDATES: readonly CandidateKind[] = [
  "LOCAL_SECURE",
  "LOCAL",
  "MDNS",
  "REMOTE_FORWARDED",
  "CLOUD",
];

export interface SanitizedLiveEvidence {
  schema_version: "1.0.0";
  selected_candidate: CandidateKind | null;
  operations: ReadOperation[];
  authentication_attempted: boolean;
  discovery_attempted: boolean;
  collection_attempted: boolean;
  publication_attempted: boolean;
  credentials_present: false;
  raw_identifiers_present: false;
  raw_responses_present: false;
}

export function createSanitizedLiveEvidence(input: {
  selected_candidate: CandidateKind | null;
  operations: readonly ReadOperation[];
  authentication_attempted?: boolean;
  discovery_attempted?: boolean;
  collection_attempted?: boolean;
  publication_attempted?: boolean;
}): SanitizedLiveEvidence {
  if (input.selected_candidate !== null
      && !ALLOWED_CANDIDATES.includes(input.selected_candidate)) {
    throw new CandidateError("CONFIGURATION", "Invalid sanitized evidence candidate");
  }

  for (const operation of input.operations) {
    if (!ALLOWED_READ_OPERATIONS.includes(operation)) {
      throw new CandidateError("CONFIGURATION", "Invalid sanitized evidence operation");
    }
  }

  return {
    schema_version: "1.0.0",
    selected_candidate: input.selected_candidate,
    operations: [...input.operations],
    authentication_attempted: input.authentication_attempted ?? false,
    discovery_attempted: input.discovery_attempted ?? false,
    collection_attempted: input.collection_attempted ?? false,
    publication_attempted: input.publication_attempted ?? false,
    credentials_present: false,
    raw_identifiers_present: false,
    raw_responses_present: false,
  };
}
