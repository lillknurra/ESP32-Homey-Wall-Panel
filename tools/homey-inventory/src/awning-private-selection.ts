import { lstat, readFile, realpath } from "node:fs/promises";
import { isAbsolute, relative, resolve } from "node:path";
import { CandidateError } from "./errors.js";
import { AWNING_ROLES, type AwningRole } from "./awning-model.js";

export interface Patch042PrivateSelection {
  schema_version: 1;
  purpose: "read_only_awning_operator_selection";
  generation: number;
  selected_homey_id_sha256: string;
  selections: Record<AwningRole, string>;
}

function isInside(parent: string, candidate: string): boolean {
  const rel = relative(resolve(parent), resolve(candidate));
  return rel === "" || (!rel.startsWith("..") && !isAbsolute(rel));
}

function validateDeviceAlias(value: unknown, role: AwningRole): string {
  if (typeof value !== "string" || !/^device_[0-9a-f]{12}$/.test(value)) {
    throw new CandidateError("CONFIGURATION", `Patch042 ${role} selection must be a canonical device alias`);
  }
  return value;
}

export function validatePatch042PrivateSelection(
  value: unknown,
  expectedHomeyDigest: string,
  expectedGeneration: number,
): Patch042PrivateSelection {
  if (!value || typeof value !== "object" || Array.isArray(value)) {
    throw new CandidateError("CONFIGURATION", "Patch042 private selection must be an object");
  }
  if (!/^[0-9a-f]{64}$/.test(expectedHomeyDigest)) {
    throw new CandidateError("CONFIGURATION", "Patch042 expected Homey digest is invalid");
  }
  if (!Number.isInteger(expectedGeneration) || expectedGeneration <= 0) {
    throw new CandidateError("CONFIGURATION", "Patch042 expected generation must be positive");
  }

  const record = value as Record<string, unknown>;
  const expectedKeys = ["generation", "purpose", "schema_version", "selected_homey_id_sha256", "selections"].sort();
  if (JSON.stringify(Object.keys(record).sort()) !== JSON.stringify(expectedKeys)) {
    throw new CandidateError("CONFIGURATION", "Patch042 private selection contains unknown or missing fields");
  }
  if (record.schema_version !== 1 || record.purpose !== "read_only_awning_operator_selection") {
    throw new CandidateError("CONFIGURATION", "Patch042 private selection schema or purpose mismatch");
  }
  if (record.generation !== expectedGeneration) {
    throw new CandidateError("CONFIGURATION", "Patch042 private selection generation mismatch");
  }
  if (record.selected_homey_id_sha256 !== expectedHomeyDigest) {
    throw new CandidateError("CONFIGURATION", "Patch042 private selection selected-Homey mismatch");
  }
  if (!record.selections || typeof record.selections !== "object" || Array.isArray(record.selections)) {
    throw new CandidateError("CONFIGURATION", "Patch042 private selection selections must be an object");
  }

  const rawSelections = record.selections as Record<string, unknown>;
  if (JSON.stringify(Object.keys(rawSelections).sort()) !== JSON.stringify([...AWNING_ROLES].sort())) {
    throw new CandidateError("CONFIGURATION", "Patch042 private selection must contain exactly three awning roles");
  }
  const selections = {} as Record<AwningRole, string>;
  const unique = new Set<string>();
  for (const role of AWNING_ROLES) {
    const alias = validateDeviceAlias(rawSelections[role], role);
    if (unique.has(alias)) {
      throw new CandidateError("CONFIGURATION", "Patch042 private selection must choose three unique devices");
    }
    unique.add(alias);
    selections[role] = alias;
  }

  return {
    schema_version: 1,
    purpose: "read_only_awning_operator_selection",
    generation: expectedGeneration,
    selected_homey_id_sha256: expectedHomeyDigest,
    selections,
  };
}

export async function loadPatch042PrivateSelection(
  selectionPath: string,
  repositoryRoot: string,
  expectedHomeyDigest: string,
  expectedGeneration: number,
): Promise<Patch042PrivateSelection> {
  if (!isAbsolute(selectionPath)) {
    throw new CandidateError("CONFIGURATION", "Patch042 private selection path must be absolute");
  }
  let directStat;
  try {
    directStat = await lstat(selectionPath);
  } catch (error) {
    throw new CandidateError("CONFIGURATION", "Patch042 private selection is unavailable", { cause: error });
  }
  if (!directStat.isFile() || directStat.isSymbolicLink()) {
    throw new CandidateError("CONFIGURATION", "Patch042 private selection must be a regular non-symlink file");
  }
  if ((directStat.mode & 0o077) !== 0) {
    throw new CandidateError("CONFIGURATION", "Patch042 private selection permissions must not allow group or other access");
  }

  let canonicalSelection: string;
  let canonicalRepository: string;
  try {
    canonicalSelection = await realpath(selectionPath);
    canonicalRepository = await realpath(repositoryRoot);
  } catch (error) {
    throw new CandidateError("CONFIGURATION", "Patch042 private selection path resolution failed", { cause: error });
  }
  if (isInside(canonicalRepository, canonicalSelection)) {
    throw new CandidateError("CONFIGURATION", "Patch042 private selection must remain outside the repository");
  }

  let parsed: unknown;
  try {
    parsed = JSON.parse(await readFile(canonicalSelection, "utf8"));
  } catch (error) {
    throw new CandidateError("CONFIGURATION", "Patch042 private selection could not be read", { cause: error });
  }
  return validatePatch042PrivateSelection(parsed, expectedHomeyDigest, expectedGeneration);
}
