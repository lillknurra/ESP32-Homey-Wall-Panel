import { execFile } from "node:child_process";
import { promisify } from "node:util";
import { CandidateError } from "./errors.js";
import type { PrivateLiveConfig } from "./private-config.js";

const execFileAsync = promisify(execFile);

export interface CredentialMaterial { readonly token: string; }
export interface CredentialProvider { load(): Promise<CredentialMaterial>; }
export type SafeExecFile = (file: string, args: readonly string[]) => Promise<{ stdout: string }>;

const realExec: SafeExecFile = async (file, args) => {
  const result = await execFileAsync(file, [...args], { encoding: "utf8", maxBuffer: 65536 });
  return { stdout: result.stdout };
};

export class MacOSKeychainCredentialProvider implements CredentialProvider {
  constructor(private readonly service: string, private readonly account: string,
              private readonly execute: SafeExecFile = realExec) {}
  async load(): Promise<CredentialMaterial> {
    try {
      const { stdout } = await this.execute("/usr/bin/security", [
        "find-generic-password", "-w", "-s", this.service, "-a", this.account,
      ]);
      const token = stdout.trim();
      if (!token) throw new Error("empty credential");
      return { token };
    } catch (error) {
      throw new CandidateError("AUTHENTICATION", "Unable to load credential from macOS Keychain", { cause: error });
    }
  }
}

export class EnvironmentCredentialProvider implements CredentialProvider {
  constructor(private readonly variableName: string,
              private readonly environment: NodeJS.ProcessEnv = process.env) {}
  async load(): Promise<CredentialMaterial> {
    const token = this.environment[this.variableName]?.trim();
    if (!token) throw new CandidateError("AUTHENTICATION", "Configured credential environment variable is unavailable");
    return { token };
  }
}

export function createCredentialProvider(config: PrivateLiveConfig): CredentialProvider {
  if (config.credential_provider === "macos-keychain") {
    return new MacOSKeychainCredentialProvider(config.keychain_service!, config.keychain_account!);
  }
  return new EnvironmentCredentialProvider(config.environment_variable!);
}
