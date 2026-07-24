import { access } from "node:fs/promises";
import { constants } from "node:fs";
import { loadPrivateLiveConfig } from "./private-config.js";
import { runPrivatePreflight } from "./preflight.js";
import { createSanitizedPreflightEvidence } from "./preflight-evidence.js";
import { checkCredentialEntryExists } from "./credential-provider.js";

export interface ControlledPreflightDependencies {
  readonly checkEntry?: typeof checkCredentialEntryExists;
}

export async function runControlledPreflight(
  configPath: string,
  repositoryRoot: string,
  dependencies: ControlledPreflightDependencies = {},
) {
  const config = await loadPrivateLiveConfig(configPath, repositoryRoot);
  const report = await runPrivatePreflight(configPath, repositoryRoot, config);
  const checkEntry = dependencies.checkEntry ?? checkCredentialEntryExists;
  let providerAvailable = true;
  let entryExists = false;
  if (config.credential_provider === "macos-keychain") {
    try { await access("/usr/bin/security", constants.X_OK); } catch { providerAvailable = false; }
    if (providerAvailable) {
      entryExists = await checkEntry(config);
    }
  } else {
    return createSanitizedPreflightEvidence(config.credential_provider, {
      config_valid: true,
      config_outside_repository: report.config_outside_repository,
      permissions_restrictive: report.restrictive_permissions,
      provider_available: false,
      credential_entry_exists: false,
    });
  }
  return createSanitizedPreflightEvidence(config.credential_provider, {
    config_valid: true,
    config_outside_repository: report.config_outside_repository,
    permissions_restrictive: report.restrictive_permissions,
    provider_available: providerAvailable,
    credential_entry_exists: entryExists,
  });
}

async function main(): Promise<void> {
  const [, , mode, configPath, repositoryRoot] = process.argv;
  if (mode !== "--preflight-only" || !configPath || !repositoryRoot) {
    throw new Error("Controlled preflight requires --preflight-only, private config path, and repository root");
  }
  const evidence = await runControlledPreflight(configPath, repositoryRoot);
  process.stdout.write(`${JSON.stringify(evidence, null, 2)}\n`);
}

if (process.argv[1]?.endsWith("preflight-runner.js")) {
  main().catch((error: unknown) => {
    const message = error instanceof Error ? error.message : "controlled preflight failed";
    process.stderr.write(`REFUSED: ${message}\n`);
    process.exitCode = 2;
  });
}
