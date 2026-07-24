import { readFile } from "node:fs/promises";
import { isAbsolute, relative, resolve } from "node:path";
import { CandidateError } from "./errors.js";

export type CredentialProvider = "macos-keychain" | "environment";

export interface PrivateLiveConfig {
  connection: "auto" | "local" | "cloud";
  credential_provider: CredentialProvider;
  keychain_service?: string;
  keychain_account?: string;
  environment_variable?: string;
  homey_selector?: string;
}

function isInside(parent: string, candidate: string): boolean {
  const rel = relative(resolve(parent), resolve(candidate));
  return rel === "" || (!rel.startsWith("..") && !isAbsolute(rel));
}

export async function loadPrivateLiveConfig(
  configPath: string,
  repositoryRoot: string,
): Promise<PrivateLiveConfig> {
  if (!isAbsolute(configPath)) {
    throw new CandidateError("CONFIGURATION", "Private config path must be absolute");
  }
  if (isInside(repositoryRoot, configPath)) {
    throw new CandidateError("CONFIGURATION", "Private config must be outside the repository");
  }

  let parsed: unknown;
  try {
    parsed = JSON.parse(await readFile(configPath, "utf8"));
  } catch (error) {
    throw new CandidateError("CONFIGURATION", "Unable to read private live configuration", { cause: error });
  }

  if (!parsed || typeof parsed !== "object" || Array.isArray(parsed)) {
    throw new CandidateError("CONFIGURATION", "Private live configuration must be an object");
  }

  const value = parsed as Record<string, unknown>;
  if (!["auto", "local", "cloud"].includes(String(value.connection))) {
    throw new CandidateError("CONFIGURATION", "Invalid private connection mode");
  }
  if (!["macos-keychain", "environment"].includes(String(value.credential_provider))) {
    throw new CandidateError("CONFIGURATION", "Invalid credential provider");
  }

  const config: PrivateLiveConfig = {
    connection: value.connection as PrivateLiveConfig["connection"],
    credential_provider: value.credential_provider as CredentialProvider,
  };

  for (const key of ["keychain_service", "keychain_account", "environment_variable", "homey_selector"] as const) {
    if (value[key] !== undefined) {
      if (typeof value[key] !== "string" || value[key].length === 0) {
        throw new CandidateError("CONFIGURATION", `Invalid ${key}`);
      }
      config[key] = value[key];
    }
  }

  if (config.credential_provider === "macos-keychain"
      && (!config.keychain_service || !config.keychain_account)) {
    throw new CandidateError("CONFIGURATION", "Keychain provider requires service and account");
  }
  if (config.credential_provider === "environment" && !config.environment_variable) {
    throw new CandidateError("CONFIGURATION", "Environment provider requires variable name");
  }

  return config;
}
