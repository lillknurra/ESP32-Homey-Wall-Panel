import { createHash } from "node:crypto";
import { CandidateError } from "./errors.js";
import {
  assertPatch039ObservedAddressPolicy,
  normalizePatch039HomeyAddress,
} from "./awning-private-config.js";

const STRICT_READ_ONLY_MANAGER_PREFIXES = Object.freeze([
  "/api/manager/devices",
  "/api/manager/flow",
] as const);

export const PATCH040_STRICT_LOCAL_TRANSPORT_CONTRACT = Object.freeze({
  homey_api_version: "3.19.1",
  node_fetch_version: "2.7.0",
  initial_probe_path: "/api/manager/system/ping",
  redirect: "manual",
  socket_policy: "ERR_SOCKET_SESSION_NOT_READY_LOCAL_FALLBACK",
  allowed_manager_prefixes: STRICT_READ_ONLY_MANAGER_PREFIXES,
  mutation_surface: "none",
});

export interface Patch039HeadersLike {
  get(name: string): string | null;
}

export interface Patch039FetchResponseLike {
  readonly url: string;
  readonly redirected: boolean;
  readonly status: number;
  readonly ok: boolean;
  readonly statusText?: string;
  readonly headers: Patch039HeadersLike;
  text(): Promise<string>;
}

export interface Patch039UtilLike {
  fetch(
    url: string,
    options?: Record<string, unknown>,
    timeoutDuration?: number,
    timeoutMessage?: string,
    patchOptions?: (options: Record<string, unknown>, url: string) => Record<string, unknown> | void,
  ): Promise<Patch039FetchResponseLike>;
}

export type Patch039HomeyAPIV3LocalConstructor = new (...args: any[]) => any;

export interface Patch039HomeyApiRuntime {
  HomeyAPIV3Local: Patch039HomeyAPIV3LocalConstructor;
  Util: Patch039UtilLike;
}

export type Patch039PersonalAccessTokenProvider = () => Promise<string>;

export interface Patch039StrictIdentity {
  readonly configuredOrigin: string;
  readonly observedOrigin: string;
  readonly homeyId: string;
  readonly homeyVersion: string | null;
}

interface StrictCallInput {
  $timeout?: number;
  method?: unknown;
  headers?: unknown;
  path?: unknown;
  body?: unknown;
}

const REDIRECT_STATUSES = new Set([301, 302, 303, 307, 308]);
const DEFAULT_TIMEOUT_MS = 5000;

function makeCandidateError(
  failureClass: ConstructorParameters<typeof CandidateError>[0],
  message: string,
  cause?: unknown,
): CandidateError {
  return cause === undefined
    ? new CandidateError(failureClass, message)
    : new CandidateError(failureClass, message, { cause });
}

function assertExpectedHomeyDigest(value: string): void {
  if (!/^[0-9a-f]{64}$/.test(value)) {
    throw new CandidateError("CONFIGURATION", "Patch039 expected Homey digest is invalid");
  }
}

function assertRequestOrigin(configuredOrigin: string, requestUrl: string): void {
  let observed: string;
  try {
    observed = new URL(requestUrl).origin;
  } catch (error) {
    throw makeCandidateError("CONFIGURATION", "Patch039 strict request URL is invalid", error);
  }
  assertPatch039ObservedAddressPolicy(configuredOrigin, observed);
}

function assertStrictResponseTarget(
  configuredOrigin: string,
  response: Patch039FetchResponseLike,
): string {
  let observedOrigin: string;
  try {
    observedOrigin = new URL(response.url).origin;
  } catch (error) {
    throw makeCandidateError("MALFORMED_RESPONSE", "Patch039 response URL is unavailable", error);
  }

  assertPatch039ObservedAddressPolicy(configuredOrigin, observedOrigin);

  if (response.redirected !== false) {
    throw new CandidateError("REACHABILITY", "Patch039 redirected response is forbidden");
  }
  if (REDIRECT_STATUSES.has(response.status) || (response.status >= 300 && response.status < 400)) {
    throw new CandidateError("REACHABILITY", "Patch039 redirect response is forbidden");
  }

  return observedOrigin;
}

function normalizeHeaders(value: unknown): Record<string, string> {
  if (value === undefined || value === null) return {};
  if (typeof value !== "object" || Array.isArray(value)) {
    throw new CandidateError("CONFIGURATION", "Patch039 manager headers are invalid");
  }

  const headers: Record<string, string> = {};
  for (const [key, rawValue] of Object.entries(value as Record<string, unknown>)) {
    if (typeof rawValue !== "string") {
      throw new CandidateError("CONFIGURATION", "Patch039 manager header value is invalid");
    }
    const lower = key.toLowerCase();
    if (["authorization", "cookie", "proxy-authorization", "host", "x-homey-id"].includes(lower)) {
      throw new CandidateError("AUTHORIZATION", "Patch039 caller-supplied credential or target headers are forbidden");
    }
    headers[key] = rawValue;
  }
  return headers;
}

function assertReadOnlyManagerPath(path: string): void {
  if (!path.startsWith("/api/manager/")) {
    throw new CandidateError("AUTHORIZATION", "Patch039 non-manager path is forbidden");
  }
  const allowed = STRICT_READ_ONLY_MANAGER_PREFIXES.some(
    (prefix) => path === prefix || path.startsWith(`${prefix}/`) || path.startsWith(`${prefix}?`),
  );
  if (!allowed) {
    throw new CandidateError("AUTHORIZATION", "Patch039 manager path is outside the read-only boundary");
  }
}

async function parseStrictResponse(response: Patch039FetchResponseLike): Promise<unknown> {
  if (response.status === 204) return undefined;
  if (!response.ok) {
    if (response.status === 401) {
      throw new CandidateError("AUTHENTICATION", "Patch039 authenticated Homey read was refused");
    }
    if (response.status === 403) {
      throw new CandidateError("AUTHORIZATION", "Patch039 authenticated Homey read is not authorized");
    }
    throw new CandidateError("MALFORMED_RESPONSE", "Patch039 Homey read returned a non-success status");
  }

  const text = await response.text();
  const contentType = response.headers.get("Content-Type");
  if (contentType?.toLowerCase().startsWith("application/json")) {
    try {
      return JSON.parse(text);
    } catch (error) {
      throw makeCandidateError("MALFORMED_RESPONSE", "Patch039 Homey read returned invalid JSON", error);
    }
  }
  return text;
}

async function loadPatch039HomeyApiRuntime(): Promise<Patch039HomeyApiRuntime> {
  const moduleName = ["homey", "api"].join("-");
  const imported = await import(moduleName) as Record<string, unknown> & {
    default?: Record<string, unknown>;
  };
  const root = imported.default && typeof imported.default === "object"
    ? { ...imported.default, ...imported }
    : imported;
  const HomeyAPIV3Local = root.HomeyAPIV3Local;
  const Util = root.Util;
  if (typeof HomeyAPIV3Local !== "function" || !Util || typeof (Util as Patch039UtilLike).fetch !== "function") {
    throw new CandidateError("API_INCOMPATIBILITY", "Pinned Homey API strict transport exports are unavailable");
  }
  return {
    HomeyAPIV3Local: HomeyAPIV3Local as Patch039HomeyAPIV3LocalConstructor,
    Util: Util as Patch039UtilLike,
  };
}

export async function probePatch039StrictIdentity(input: {
  configuredAddress: string;
  expectedHomeyDigest: string;
  util: Patch039UtilLike;
  timeoutMs?: number;
}): Promise<Patch039StrictIdentity> {
  const configuredOrigin = normalizePatch039HomeyAddress(input.configuredAddress);
  assertExpectedHomeyDigest(input.expectedHomeyDigest);

  const requestUrl = new URL(PATCH040_STRICT_LOCAL_TRANSPORT_CONTRACT.initial_probe_path, configuredOrigin).toString();
  assertRequestOrigin(configuredOrigin, requestUrl);

  const response = await input.util.fetch(
    requestUrl,
    {
      method: "GET",
      headers: {},
      redirect: "manual",
    },
    input.timeoutMs ?? DEFAULT_TIMEOUT_MS,
    "Patch039 strict identity ping timed out",
  );

  const observedOrigin = assertStrictResponseTarget(configuredOrigin, response);
  if (!response.ok) {
    throw new CandidateError("REACHABILITY", "Patch039 unauthenticated identity ping did not succeed");
  }

  const homeyId = response.headers.get("X-Homey-ID");
  if (!homeyId) {
    throw new CandidateError("SCHEMA_MISMATCH", "Patch039 identity ping did not return X-Homey-ID");
  }
  const actualDigest = createHash("sha256").update(homeyId, "utf8").digest("hex");
  if (actualDigest !== input.expectedHomeyDigest) {
    throw new CandidateError("AUTHORIZATION", "Patch039 identity ping Homey does not match the private selection");
  }

  return {
    configuredOrigin,
    observedOrigin,
    homeyId,
    homeyVersion: response.headers.get("X-Homey-Version"),
  };
}

function createPatch039StrictHomeySubclass(input: {
  runtime: Patch039HomeyApiRuntime;
  identity: Patch039StrictIdentity;
  personalAccessToken: string;
}): Patch039HomeyAPIV3LocalConstructor {
  if (!input.personalAccessToken) {
    throw new CandidateError("AUTHENTICATION", "Patch039 Personal Access Token is unavailable");
  }
  const Base = input.runtime.HomeyAPIV3Local;
  const util = input.runtime.Util;
  const configuredOrigin = input.identity.configuredOrigin;
  const rawHomeyId = input.identity.homeyId;
  const personalAccessToken = input.personalAccessToken;

  return class Patch039StrictHomeyAPIV3Local extends Base {
    async __apiRequest(_request: unknown): Promise<never> {
      const error = new Error("Patch039 socket transport is disabled") as Error & { code?: string };
      error.code = "ERR_SOCKET_SESSION_NOT_READY";
      throw error;
    }

    async connect(): Promise<never> {
      throw new CandidateError("AUTHORIZATION", "Patch039 socket connect is forbidden");
    }

    async subscribe(): Promise<never> {
      throw new CandidateError("AUTHORIZATION", "Patch039 subscription transport is forbidden");
    }

    async login(): Promise<never> {
      throw new CandidateError("AUTHORIZATION", "Patch039 login and token refresh are forbidden");
    }

    async discoverBaseUrl(): Promise<never> {
      throw new CandidateError("AUTHORIZATION", "Patch039 Homey discovery is forbidden");
    }

    async call(request: StrictCallInput): Promise<unknown> {
      const method = String(request.method ?? "").toUpperCase();
      if (method !== "GET") {
        throw new CandidateError("AUTHORIZATION", "Patch039 strict transport permits GET only");
      }
      if (request.body !== undefined && request.body !== null) {
        throw new CandidateError("AUTHORIZATION", "Patch039 strict transport forbids request bodies");
      }
      if (typeof request.path !== "string") {
        throw new CandidateError("CONFIGURATION", "Patch039 manager path is unavailable");
      }
      assertReadOnlyManagerPath(request.path);

      const headers = normalizeHeaders(request.headers);
      headers["X-Homey-ID"] = rawHomeyId;
      headers.Authorization = `Bearer ${personalAccessToken}`;

      const requestUrl = `${configuredOrigin}${request.path}`;
      assertRequestOrigin(configuredOrigin, requestUrl);
      const normalizedRequestUrl = new URL(requestUrl);
      if (normalizedRequestUrl.hash) {
        throw new CandidateError("AUTHORIZATION", "Patch039 manager URL fragment is forbidden");
      }
      assertReadOnlyManagerPath(`${normalizedRequestUrl.pathname}${normalizedRequestUrl.search}`);

      const response = await util.fetch(
        requestUrl,
        {
          method: "GET",
          headers,
          redirect: "manual",
        },
        request.$timeout ?? DEFAULT_TIMEOUT_MS,
        "Patch039 strict manager read timed out",
      );

      assertStrictResponseTarget(configuredOrigin, response);
      return parseStrictResponse(response);
    }
  };
}

export interface Patch039StrictRawReadSurface {
  readonly id: string;
  readonly devices: {
    getDevices(): Promise<unknown>;
    getCapabilityValue(input: { deviceId: string; capabilityId: string }): Promise<unknown>;
  };
  readonly flow: {
    getFlows(): Promise<unknown>;
    getFlowCardActions(): Promise<unknown>;
    getAdvancedFlows(): Promise<unknown>;
  };
}

export async function createPatch039StrictLocalRawApi(input: {
  configuredAddress: string;
  expectedHomeyDigest: string;
  getPersonalAccessToken: Patch039PersonalAccessTokenProvider;
  timeoutMs?: number;
  runtime?: Patch039HomeyApiRuntime;
}): Promise<Patch039StrictRawReadSurface> {
  const configuredOrigin = normalizePatch039HomeyAddress(input.configuredAddress);
  assertExpectedHomeyDigest(input.expectedHomeyDigest);
  const runtime = input.runtime ?? await loadPatch039HomeyApiRuntime();

  const identity = await probePatch039StrictIdentity({
    configuredAddress: configuredOrigin,
    expectedHomeyDigest: input.expectedHomeyDigest,
    util: runtime.Util,
    ...(input.timeoutMs === undefined ? {} : { timeoutMs: input.timeoutMs }),
  });

  const personalAccessToken = (await input.getPersonalAccessToken()).trim();
  if (!personalAccessToken) {
    throw new CandidateError("AUTHENTICATION", "Patch039 Personal Access Token is unavailable");
  }

  const StrictHomeyAPIV3Local = createPatch039StrictHomeySubclass({
    runtime,
    identity,
    personalAccessToken,
  });

  const strictHomey = new StrictHomeyAPIV3Local({
    properties: {
      id: identity.homeyId,
      softwareVersion: identity.homeyVersion,
    },
    baseUrl: configuredOrigin,
    strategy: [],
    reconnect: false,
    api: undefined,
    debug: false,
  });

  return Object.freeze({
    id: identity.homeyId,
    devices: Object.freeze({
      getDevices: () => strictHomey.devices.getDevices(),
      getCapabilityValue: (request: { deviceId: string; capabilityId: string }) =>
        strictHomey.devices.getCapabilityValue(request),
    }),
    flow: Object.freeze({
      getFlows: () => strictHomey.flow.getFlows(),
      getFlowCardActions: () => strictHomey.flow.getFlowCardActions(),
      getAdvancedFlows: () => strictHomey.flow.getAdvancedFlows(),
    }),
  });
}
