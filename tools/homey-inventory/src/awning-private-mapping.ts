import { lstat, readFile, realpath, rename, rm, writeFile } from "node:fs/promises";
import { isAbsolute, relative, resolve } from "node:path";
import { CandidateError } from "./errors.js";
import { AWNING_ROLES, type AwningRole } from "./awning-model.js";

export interface PrivateAwningMapping {
  schema_version: 1;
  purpose: "read_only_awning_evidence";
  generation: number;
  selected_homey_id_sha256: string;
  mappings: Record<AwningRole, string>;
}

function isInside(parent: string, candidate: string): boolean {
  const rel = relative(resolve(parent), resolve(candidate));
  return rel === "" || (!rel.startsWith("..") && !isAbsolute(rel));
}

function requireDigest(value: unknown, fieldName: string): string {
  if (typeof value !== "string" || !/^[0-9a-f]{64}$/.test(value)) {
    throw new CandidateError("CONFIGURATION", `${fieldName} must be lowercase SHA-256 hex`);
  }
  return value;
}

export function validatePrivateAwningMapping(value: unknown): PrivateAwningMapping {
  if (!value || typeof value !== "object" || Array.isArray(value)) {
    throw new CandidateError("CONFIGURATION", "Private awning mapping must be an object");
  }
  const record = value as Record<string, unknown>;
  const topKeys = Object.keys(record).sort();
  const expectedTopKeys = ["generation", "mappings", "purpose", "schema_version", "selected_homey_id_sha256"].sort();
  if (JSON.stringify(topKeys) !== JSON.stringify(expectedTopKeys)) {
    throw new CandidateError("CONFIGURATION", "Private awning mapping contains unknown or missing fields");
  }
  if (record.schema_version !== 1) throw new CandidateError("CONFIGURATION", "Private awning mapping schema_version must be 1");
  if (record.purpose !== "read_only_awning_evidence") {
    throw new CandidateError("CONFIGURATION", "Private awning mapping purpose mismatch");
  }
  if (!Number.isInteger(record.generation) || Number(record.generation) <= 0) {
    throw new CandidateError("CONFIGURATION", "Private awning mapping generation must be a positive integer");
  }
  const selectedDigest = requireDigest(record.selected_homey_id_sha256, "selected_homey_id_sha256");
  if (!record.mappings || typeof record.mappings !== "object" || Array.isArray(record.mappings)) {
    throw new CandidateError("CONFIGURATION", "Private awning mapping mappings must be an object");
  }
  const rawMappings = record.mappings as Record<string, unknown>;
  const keys = Object.keys(rawMappings).sort();
  const expectedKeys = [...AWNING_ROLES].sort();
  if (JSON.stringify(keys) !== JSON.stringify(expectedKeys)) {
    throw new CandidateError("CONFIGURATION", "Private awning mapping must contain exactly awning_1, awning_2, and awning_3");
  }
  const mappings = {} as Record<AwningRole, string>;
  const seen = new Set<string>();
  for (const role of AWNING_ROLES) {
    const rawId = rawMappings[role];
    if (typeof rawId !== "string" || rawId.length === 0 || rawId.length > 256) {
      throw new CandidateError("CONFIGURATION", "Private awning mapping contains an invalid device correlation");
    }
    if (seen.has(rawId)) {
      throw new CandidateError("CONFIGURATION", "Private awning mapping devices must be unique");
    }
    seen.add(rawId);
    mappings[role] = rawId;
  }
  return {
    schema_version: 1,
    purpose: "read_only_awning_evidence",
    generation: Number(record.generation),
    selected_homey_id_sha256: selectedDigest,
    mappings,
  };
}

export async function loadPrivateAwningMapping(
  mappingPath: string,
  repositoryRoot: string,
): Promise<PrivateAwningMapping> {
  if (!isAbsolute(mappingPath)) {
    throw new CandidateError("CONFIGURATION", "Private awning mapping path must be absolute");
  }
  let canonicalMapping: string;
  let canonicalRepository: string;
  try {
    canonicalMapping = await realpath(mappingPath);
    canonicalRepository = await realpath(repositoryRoot);
  } catch (error) {
    throw new CandidateError("CONFIGURATION", "Unable to resolve private awning mapping paths", { cause: error });
  }
  if (isInside(canonicalRepository, canonicalMapping)) {
    throw new CandidateError("CONFIGURATION", "Private awning mapping must be outside the repository");
  }
  const stat = await lstat(canonicalMapping);
  if (!stat.isFile()) throw new CandidateError("CONFIGURATION", "Private awning mapping must be a regular file");
  if ((stat.mode & 0o077) !== 0) {
    throw new CandidateError("CONFIGURATION", "Private awning mapping permissions must not allow group or other access");
  }
  let parsed: unknown;
  try {
    parsed = JSON.parse(await readFile(canonicalMapping, "utf8"));
  } catch (error) {
    throw new CandidateError("CONFIGURATION", "Unable to read private awning mapping", { cause: error });
  }
  return validatePrivateAwningMapping(parsed);
}

export function assertMappingMatchesSelectedHomey(
  mapping: PrivateAwningMapping,
  expectedHomeyDigest: string,
): void {
  if (mapping.selected_homey_id_sha256 !== expectedHomeyDigest) {
    throw new CandidateError("CONFIGURATION", "Private awning mapping selected-Homey mismatch");
  }
}

export function assertMappedDevicesFresh(
  mapping: PrivateAwningMapping,
  observedRawDeviceIds: readonly string[],
): void {
  const counts = new Map<string, number>();
  for (const id of observedRawDeviceIds) counts.set(id, (counts.get(id) ?? 0) + 1);
  for (const role of AWNING_ROLES) {
    if (counts.get(mapping.mappings[role]) !== 1) {
      throw new CandidateError("SCHEMA_MISMATCH", "Private awning mapping is stale or ambiguous");
    }
  }
}

export async function savePrivateAwningMapping(
  mappingPath: string,
  repositoryRoot: string,
  mapping: PrivateAwningMapping,
): Promise<void> {
  if (!isAbsolute(mappingPath)) {
    throw new CandidateError("CONFIGURATION", "Private awning mapping path must be absolute");
  }
  const validated = validatePrivateAwningMapping(mapping);
  const parent = resolve(mappingPath, "..");
  let canonicalParent: string;
  let canonicalRepository: string;
  try {
    canonicalParent = await realpath(parent);
    canonicalRepository = await realpath(repositoryRoot);
  } catch (error) {
    throw new CandidateError("CONFIGURATION", "Unable to resolve private awning mapping parent", { cause: error });
  }
  const target = resolve(canonicalParent, mappingPath.split(/[\\/]/).pop()!);
  if (isInside(canonicalRepository, target)) {
    throw new CandidateError("CONFIGURATION", "Private awning mapping must be outside the repository");
  }
  try {
    const existing = await lstat(target);
    if (!existing.isFile() || existing.isSymbolicLink()) {
      throw new CandidateError("CONFIGURATION", "Private awning mapping target must be a regular file");
    }
    if ((existing.mode & 0o077) !== 0) {
      throw new CandidateError("CONFIGURATION", "Existing private awning mapping permissions are too broad");
    }
  } catch (error) {
    const code = (error as NodeJS.ErrnoException).code;
    if (code !== "ENOENT") throw error;
  }

  const tmp = `${target}.tmp-${process.pid}`;
  const serialized = `${JSON.stringify(validated, null, 2)}\n`;
  try {
    await writeFile(tmp, serialized, { mode: 0o600, flag: "wx" });
    await rename(tmp, target);
    const readback = await loadPrivateAwningMapping(target, repositoryRoot);
    if (JSON.stringify(readback) !== JSON.stringify(validated)) {
      throw new CandidateError("CONFIGURATION", "Private awning mapping readback mismatch");
    }
  } finally {
    await rm(tmp, { force: true });
  }
}
