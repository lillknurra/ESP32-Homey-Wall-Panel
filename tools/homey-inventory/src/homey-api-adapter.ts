import { CandidateError } from "./errors.js";
import type { CandidateConnector, ReadonlyHomeyClient } from "./readonly-client.js";
import type { CandidateKind } from "./model.js";
import type { PrivateLiveConfig } from "./private-config.js";
import type { CredentialProvider } from "./credential-provider.js";

export interface LiveConnectorOptions {
  liveEnabled: boolean;
  privateConfig?: PrivateLiveConfig | undefined;
  credentialProvider?: CredentialProvider | undefined;
  compatibilityValidated?: boolean | undefined;
}

export class HomeyApiConnector implements CandidateConnector {
  constructor(public readonly kind: CandidateKind, private readonly options: LiveConnectorOptions) {}
  async connect(_timeoutMs: number): Promise<ReadonlyHomeyClient> {
    if (!this.options.liveEnabled) throw new CandidateError("CONFIGURATION", "Live Homey access requires explicit --live");
    if (!this.options.privateConfig) throw new CandidateError("CONFIGURATION", "Live Homey access requires validated private configuration");
    if (!this.options.credentialProvider) throw new CandidateError("CONFIGURATION", "Live Homey access requires an explicit credential provider");
    if (!this.options.compatibilityValidated) throw new CandidateError("API_INCOMPATIBILITY", "Pinned Homey API compatibility must be validated before connection");
    throw new CandidateError("API_INCOMPATIBILITY", `Live ${this.kind} construction remains disabled until the authorized live phase`);
  }
}
