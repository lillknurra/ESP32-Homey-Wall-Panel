import { lstat, realpath } from "node:fs/promises";
import { isAbsolute, relative, resolve } from "node:path";
import { CandidateError } from "./errors.js";
import type { PrivateLiveConfig } from "./private-config.js";

export interface PrivatePreflightReport {
  config_regular_file: true;
  config_outside_repository: true;
  restrictive_permissions: true;
  provider_supported: true;
}

function isInside(parent: string, candidate: string): boolean {
  const rel = relative(parent, candidate);
  return rel === "" || (!rel.startsWith("..") && !isAbsolute(rel));
}

export async function runPrivatePreflight(
  configPath: string,
  repositoryRoot: string,
  config: PrivateLiveConfig,
  platform: NodeJS.Platform = process.platform,
): Promise<PrivatePreflightReport> {
  if (!isAbsolute(configPath) || !isAbsolute(repositoryRoot)) {
    throw new CandidateError("CONFIGURATION", "Preflight paths must be absolute");
  }

  let canonicalConfig: string;
  let canonicalRepository: string;
  try {
    canonicalConfig = await realpath(resolve(configPath));
    canonicalRepository = await realpath(resolve(repositoryRoot));
  } catch (error) {
    throw new CandidateError("CONFIGURATION", "Unable to resolve private preflight paths", { cause: error });
  }

  if (isInside(canonicalRepository, canonicalConfig)) {
    throw new CandidateError("CONFIGURATION", "Private config must be outside the repository");
  }

  let stat;
  try {
    stat = await lstat(canonicalConfig);
  } catch (error) {
    throw new CandidateError("CONFIGURATION", "Unable to inspect private config", { cause: error });
  }

  if (!stat.isFile()) {
    throw new CandidateError("CONFIGURATION", "Private config must be a regular file");
  }
  if ((stat.mode & 0o077) !== 0) {
    throw new CandidateError("CONFIGURATION", "Private config permissions must not allow group or other access");
  }
  if (config.credential_provider === "macos-keychain" && platform !== "darwin") {
    throw new CandidateError("CONFIGURATION", "macOS Keychain provider requires macOS");
  }

  return {
    config_regular_file: true,
    config_outside_repository: true,
    restrictive_permissions: true,
    provider_supported: true,
  };
}
