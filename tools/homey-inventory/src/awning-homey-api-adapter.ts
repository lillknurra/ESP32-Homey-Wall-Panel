import { createHash } from "node:crypto";
import { CandidateError } from "./errors.js";
import { Patch039CallLedger } from "./awning-call-ledger.js";
import type { Patch039ReadonlyHomeyClient } from "./awning-readonly-client.js";
import type { Patch039SessionEvidence } from "./awning-model.js";
import { createPatch039StrictLocalRawApi, type Patch039HomeyApiRuntime } from "./patch039-strict-local-transport.js";

export const PATCH039_HOMEY_API_SOURCE_CONTRACT = Object.freeze({
  package_version: "3.19.1",
  package_integrity: "sha512-56sd8LvC/CBCOqYwph+FEg0TUY0gkjWJ33+JDlOhAtVNRiZtByj4CxGrdYYFtUx70ufs1eASs1ad3rQD7PWhKA==",
  factory: "HomeyAPI.createLocalAPI",
  initial_probe_path: "/api/manager/system/ping",
  identity_header: "X-Homey-ID",
  credential_kind: "personal_access_token",
  candidate_fallback: false,
  live_factory_wired: false,
});

interface RawManagerDevices {
  getDevices(): Promise<unknown>;
  getCapabilityValue(input: { deviceId: string; capabilityId: string }): Promise<unknown>;
}

interface RawManagerFlow {
  getFlows(): Promise<unknown>;
  getFlowCardActions(): Promise<unknown>;
  getAdvancedFlows(): Promise<unknown>;
}

export interface RawPatch039HomeyApi {
  id?: unknown;
  devices?: RawManagerDevices;
  flow?: RawManagerFlow;
}

function recordOf(value: unknown): Record<string, unknown> | null {
  return value && typeof value === "object" && !Array.isArray(value) ? value as Record<string, unknown> : null;
}

function asArray(value: unknown): unknown[] {
  if (Array.isArray(value)) return [...value];
  if (value && typeof value === "object") return Object.values(value as Record<string, unknown>);
  throw new CandidateError("MALFORMED_RESPONSE", "Patch039 Homey read returned an unsupported collection shape");
}

function homeyIdDigest(rawId: unknown): string {
  if (typeof rawId !== "string" || rawId.length === 0) {
    throw new CandidateError("SCHEMA_MISMATCH", "Patch039 connected Homey identity is unavailable");
  }
  return createHash("sha256").update(rawId, "utf8").digest("hex");
}

export function verifyPatch039SelectedHomey(rawId: unknown, expectedDigest: string): void {
  const actualDigest = homeyIdDigest(rawId);
  if (actualDigest !== expectedDigest) {
    throw new CandidateError("AUTHORIZATION", "Patch039 connected Homey identity does not match the private selection");
  }
}

function sanitizedFailureClass(error: unknown): import("./model.js").FailureClass {
  return error instanceof CandidateError ? error.failureClass : "MALFORMED_RESPONSE";
}

function pairKey(deviceId: string, capabilityId: string): string {
  return JSON.stringify([deviceId, capabilityId]);
}

function indexConditionalCapabilityValueFallbacks(devices: readonly unknown[]): Set<string> {
  const allowed = new Set<string>();
  for (const item of devices) {
    const device = recordOf(item);
    if (!device) continue;
    const rawId = device.id ?? device._id;
    if (typeof rawId !== "string" || rawId.length === 0) continue;

    const observedCapabilityIds = new Set<string>();
    if (Array.isArray(device.capabilities)) {
      for (const capabilityId of device.capabilities) {
        if (typeof capabilityId === "string" && capabilityId.length > 0) observedCapabilityIds.add(capabilityId);
      }
    }
    const capabilitiesObj = recordOf(device.capabilitiesObj);
    if (capabilitiesObj) {
      for (const capabilityId of Object.keys(capabilitiesObj)) {
        if (capabilityId.length > 0) observedCapabilityIds.add(capabilityId);
      }
    }

    for (const capabilityId of observedCapabilityIds) {
      const capability = capabilitiesObj ? recordOf(capabilitiesObj[capabilityId]) : null;
      const currentValuePresent = capability !== null
        && Object.hasOwn(capability, "value")
        && capability.value !== undefined;
      if (!currentValuePresent) allowed.add(pairKey(rawId, capabilityId));
    }
  }
  return allowed;
}

export function wrapVerifiedPatch039HomeyApi(
  rawApi: RawPatch039HomeyApi,
  expectedHomeyDigest: string,
  ledger = new Patch039CallLedger(),
): Patch039ReadonlyHomeyClient {
  verifyPatch039SelectedHomey(rawApi.id, expectedHomeyDigest);
  if (!rawApi.devices || !rawApi.flow) {
    throw new CandidateError("API_INCOMPATIBILITY", "Patch039 required Homey managers are unavailable");
  }

  const devices = rawApi.devices;
  const flow = rawApi.flow;
  let freshDeviceInventoryObserved = false;
  let conditionalCapabilityValueFallbacks = new Set<string>();
  const session: Patch039SessionEvidence = {
    authentication_attempted: false,
    authentication_succeeded: false,
    selected_homey_verified: true,
    collection_attempted: false,
    publication_attempted: false,
  };

  const authenticatedRead = async <T>(
    operation: import("./awning-call-ledger.js").Patch039ReadOperation,
    read: () => Promise<T>,
    count: (value: T) => number | null,
  ): Promise<T> => {
    session.authentication_attempted = true;
    session.collection_attempted = true;
    try {
      const result = await read();
      session.authentication_succeeded = true;
      ledger.success(operation, count(result));
      return result;
    } catch (error) {
      const failureClass = sanitizedFailureClass(error);
      ledger.failure(operation, failureClass);
      throw new CandidateError(failureClass, "Patch039 allowlisted Homey read failed", { cause: error });
    }
  };

  const client: Patch039ReadonlyHomeyClient = {
    ledger,
    async getDevices() {
      freshDeviceInventoryObserved = false;
      conditionalCapabilityValueFallbacks = new Set<string>();
      const raw = await authenticatedRead("devices.read", () => devices.getDevices(), (value) => asArray(value).length);
      const collection = asArray(raw);
      conditionalCapabilityValueFallbacks = indexConditionalCapabilityValueFallbacks(collection);
      freshDeviceInventoryObserved = true;
      return collection;
    },
    async getFlows() {
      const raw = await authenticatedRead("flows.read", () => flow.getFlows(), (value) => asArray(value).length);
      return asArray(raw);
    },
    async getFlowCardActions() {
      const raw = await authenticatedRead("flow_card_actions.read", () => flow.getFlowCardActions(), (value) => asArray(value).length);
      return asArray(raw);
    },
    async getAdvancedFlows() {
      const raw = await authenticatedRead("advanced_flows.read", () => flow.getAdvancedFlows(), (value) => asArray(value).length);
      return asArray(raw);
    },
    async getCapabilityValue(deviceId: string, capabilityId: string) {
      if (!freshDeviceInventoryObserved) {
        throw new CandidateError("AUTHORIZATION", "Patch039 capability-value fallback requires a successful fresh getDevices read");
      }
      const key = pairKey(deviceId, capabilityId);
      if (!conditionalCapabilityValueFallbacks.has(key)) {
        throw new CandidateError("AUTHORIZATION", "Patch039 capability-value fallback pair was not observed as missing current-value evidence");
      }
      conditionalCapabilityValueFallbacks.delete(key);
      return authenticatedRead(
        "capability_value.read",
        () => devices.getCapabilityValue({ deviceId, capabilityId }),
        () => null,
      );
    },
    sessionEvidence() {
      return { ...session };
    },
  };
  return Object.freeze(client);
}

export const PATCH040_STRICT_LOCAL_TRANSPORT_SOURCE_CONTRACT = Object.freeze({
  implementation: "Patch039StrictHomeyAPIV3Local",
  create_local_api_used: false,
  initial_ping_authenticated: false,
  redirect: "manual",
  socket_transport: false,
  credential_retrieval: "lazy_after_identity_gate",
});

export async function createPatch039LocalReadonlyClient(input: {
  configuredAddress: string;
  expectedHomeyDigest: string;
  getPersonalAccessToken: () => Promise<string>;
  timeoutMs?: number;
  runtime?: Patch039HomeyApiRuntime;
}): Promise<Patch039ReadonlyHomeyClient> {
  const rawApi = await createPatch039StrictLocalRawApi({
    configuredAddress: input.configuredAddress,
    expectedHomeyDigest: input.expectedHomeyDigest,
    getPersonalAccessToken: input.getPersonalAccessToken,
    ...(input.timeoutMs === undefined ? {} : { timeoutMs: input.timeoutMs }),
    ...(input.runtime === undefined ? {} : { runtime: input.runtime }),
  }) as RawPatch039HomeyApi;
  return wrapVerifiedPatch039HomeyApi(rawApi, input.expectedHomeyDigest);
}
