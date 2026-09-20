import { lstat, readFile, realpath } from "node:fs/promises";
import { isAbsolute, relative, resolve } from "node:path";
import { CandidateError } from "./errors.js";

export interface Patch039PrivateConfig {
  schema_version: 1;
  connection: "local";
  credential_kind: "personal_access_token";
  credential_provider: "macos-keychain";
  keychain_service: string;
  keychain_account: string;
  homey_address: string;
  expected_homey_id_sha256: string;
}

const CONFIG_KEYS = [
  "schema_version",
  "connection",
  "credential_kind",
  "credential_provider",
  "keychain_service",
  "keychain_account",
  "homey_address",
  "expected_homey_id_sha256",
] as const;

function isInside(parent: string, candidate: string): boolean {
  const rel = relative(resolve(parent), resolve(candidate));
  return rel === "" || (!rel.startsWith("..") && !isAbsolute(rel));
}

export function normalizePatch039HomeyAddress(value: string): string {
  let parsed: URL;
  try {
    parsed = new URL(value);
  } catch {
    throw new CandidateError("CONFIGURATION", "Patch039 Homey address is invalid");
  }
  if (parsed.protocol !== "http:" && parsed.protocol !== "https:") {
    throw new CandidateError("CONFIGURATION", "Patch039 Homey address protocol is unsupported");
  }
  if (parsed.username || parsed.password || parsed.search || parsed.hash) {
    throw new CandidateError("CONFIGURATION", "Patch039 Homey address must not contain credentials, query, or fragment");
  }
  if (parsed.pathname !== "/" && parsed.pathname !== "") {
    throw new CandidateError("CONFIGURATION", "Patch039 Homey address must identify one origin without a path");
  }
  return parsed.origin;
}

export function assertPatch039ObservedAddressPolicy(configuredAddress: string, observedAddress: string): void {
  const configured = new URL(normalizePatch039HomeyAddress(configuredAddress));
  const observed = new URL(normalizePatch039HomeyAddress(observedAddress));
  if (configured.protocol === "https:" && observed.protocol !== "https:") {
    throw new CandidateError("TLS", "Patch039 secure target downgrade is forbidden");
  }
  if (configured.origin !== observed.origin) {
    throw new CandidateError("REACHABILITY", "Patch039 redirect or target substitution is forbidden");
  }
}

export async function loadPatch039PrivateConfig(
  configPath: string,
  repositoryRoot: string,
): Promise<Patch039PrivateConfig> {
  if (!isAbsolute(configPath)) {
    throw new CandidateError("CONFIGURATION", "Patch039 private config path must be absolute");
  }

  let canonicalConfig: string;
  let canonicalRepository: string;
  try {
    canonicalConfig = await realpath(configPath);
    canonicalRepository = await realpath(repositoryRoot);
  } catch (error) {
    throw new CandidateError("CONFIGURATION", "Unable to resolve Patch039 private config paths", { cause: error });
  }

  if (isInside(canonicalRepository, canonicalConfig)) {
    throw new CandidateError("CONFIGURATION", "Patch039 private config must be outside the repository");
  }

  const stat = await lstat(canonicalConfig);
  if (!stat.isFile()) {
    throw new CandidateError("CONFIGURATION", "Patch039 private config must be a regular file");
  }
  if ((stat.mode & 0o077) !== 0) {
    throw new CandidateError("CONFIGURATION", "Patch039 private config permissions must not allow group or other access");
  }

  let parsed: unknown;
  try {
    parsed = JSON.parse(await readFile(canonicalConfig, "utf8"));
  } catch (error) {
    throw new CandidateError("CONFIGURATION", "Unable to read Patch039 private config", { cause: error });
  }
  if (!parsed || typeof parsed !== "object" || Array.isArray(parsed)) {
    throw new CandidateError("CONFIGURATION", "Patch039 private config must be an object");
  }

  const value = parsed as Record<string, unknown>;
  const unknownKeys = Object.keys(value).filter((key) => !CONFIG_KEYS.includes(key as (typeof CONFIG_KEYS)[number]));
  if (unknownKeys.length > 0) {
    throw new CandidateError("CONFIGURATION", "Patch039 private config contains unknown fields");
  }
  if (value.schema_version !== 1) throw new CandidateError("CONFIGURATION", "Patch039 private config schema_version must be 1");
  if (value.connection !== "local") throw new CandidateError("CONFIGURATION", "Patch039 connection must be local");
  if (value.credential_kind !== "personal_access_token") {
    throw new CandidateError("CONFIGURATION", "Patch039 credential_kind must be personal_access_token");
  }
  if (value.credential_provider !== "macos-keychain") {
    throw new CandidateError("CONFIGURATION", "Patch039 credential_provider must be macos-keychain");
  }
  for (const key of ["keychain_service", "keychain_account", "homey_address", "expected_homey_id_sha256"] as const) {
    if (typeof value[key] !== "string" || value[key].length === 0) {
      throw new CandidateError("CONFIGURATION", `Patch039 ${key} is required`);
    }
  }
  const expectedDigest = value.expected_homey_id_sha256 as string;
  if (!/^[0-9a-f]{64}$/.test(expectedDigest)) {
    throw new CandidateError("CONFIGURATION", "Patch039 expected_homey_id_sha256 must be lowercase SHA-256 hex");
  }

  return {
    schema_version: 1,
    connection: "local",
    credential_kind: "personal_access_token",
    credential_provider: "macos-keychain",
    keychain_service: value.keychain_service as string,
    keychain_account: value.keychain_account as string,
    homey_address: normalizePatch039HomeyAddress(value.homey_address as string),
    expected_homey_id_sha256: expectedDigest,
  };
}
