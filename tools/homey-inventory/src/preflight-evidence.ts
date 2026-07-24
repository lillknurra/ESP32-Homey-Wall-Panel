import type { CredentialProvider as CredentialProviderKind } from "./private-config.js";

export type PreflightStatus = "PASS" | "REFUSED";

export interface SanitizedPreflightEvidence {
  readonly mode: "preflight_only";
  readonly status: PreflightStatus;
  readonly provider: CredentialProviderKind;
  readonly config_valid: boolean;
  readonly config_outside_repository: boolean;
  readonly permissions_restrictive: boolean;
  readonly provider_available: boolean;
  readonly credential_entry_exists: boolean;
  readonly credential_value_read: false;
  readonly homey_client_constructed: false;
  readonly discovery_attempted: false;
  readonly authentication_attempted: false;
  readonly network_access_attempted: false;
}

export function createSanitizedPreflightEvidence(
  provider: CredentialProviderKind,
  values: Pick<SanitizedPreflightEvidence,
    "config_valid" | "config_outside_repository" | "permissions_restrictive" |
    "provider_available" | "credential_entry_exists">,
): SanitizedPreflightEvidence {
  return {
    mode: "preflight_only",
    status: Object.values(values).every(Boolean) ? "PASS" : "REFUSED",
    provider,
    ...values,
    credential_value_read: false,
    homey_client_constructed: false,
    discovery_attempted: false,
    authentication_attempted: false,
    network_access_attempted: false,
  };
}
