import assert from "node:assert/strict";
import { createHash } from "node:crypto";
import test from "node:test";
import * as strictTransportModule from "../src/patch039-strict-local-transport.js";
import {
  createPatch039StrictLocalRawApi,
  probePatch039StrictIdentity,
  type Patch039FetchResponseLike,
  type Patch039HomeyApiRuntime,
} from "../src/patch039-strict-local-transport.js";

const rawHomeyId = ["synthetic", "homey", "strict"].join("-");
const expectedDigest = createHash("sha256").update(rawHomeyId).digest("hex");
const configuredAddress = ["https:", "", "strict-homey.invalid"].join("/");
const privateToken = ["synthetic", "credential", "material"].join("-");

class HeaderBag {
  readonly #values = new Map<string, string>();
  constructor(values: Record<string, string> = {}) {
    for (const [key, value] of Object.entries(values)) this.#values.set(key.toLowerCase(), value);
  }
  get(name: string): string | null {
    return this.#values.get(name.toLowerCase()) ?? null;
  }
}

function response(input: {
  url?: string;
  status?: number;
  redirected?: boolean;
  headers?: Record<string, string>;
  body?: string;
} = {}): Patch039FetchResponseLike {
  const status = input.status ?? 200;
  return {
    url: input.url ?? `${configuredAddress}/api/manager/system/ping`,
    status,
    redirected: input.redirected ?? false,
    ok: status >= 200 && status < 300,
    statusText: "synthetic",
    headers: new HeaderBag(input.headers),
    async text() { return input.body ?? ""; },
  };
}

interface FetchCall {
  url: string;
  options: Record<string, unknown>;
}

class FakeHomeyAPIV3Local {
  id: string;
  devices: {
    getDevices(): Promise<unknown>;
    getCapabilityValue(input: { deviceId: string; capabilityId: string }): Promise<unknown>;
  };
  flow: {
    getFlows(): Promise<unknown>;
    getFlowCardActions(): Promise<unknown>;
    getAdvancedFlows(): Promise<unknown>;
  };
  socketAttempts = 0;

  constructor(options: Record<string, unknown>) {
    const properties = options.properties as Record<string, unknown>;
    this.id = String(properties.id);
    const request = async (path: string): Promise<unknown> => {
      this.socketAttempts += 1;
      try {
        await (this as unknown as { __apiRequest(input: unknown): Promise<unknown> }).__apiRequest({ path });
        throw new Error("synthetic socket path unexpectedly returned");
      } catch (error) {
        if ((error as { code?: string }).code !== "ERR_SOCKET_SESSION_NOT_READY") throw error;
      }
      return (this as unknown as { call(input: Record<string, unknown>): Promise<unknown> }).call({
        method: "GET",
        path,
      });
    };
    this.devices = {
      getDevices: () => request("/api/manager/devices/device"),
      getCapabilityValue: ({ deviceId, capabilityId }) => request(
        `/api/manager/devices/device/${encodeURIComponent(deviceId)}/capability/${encodeURIComponent(capabilityId)}`,
      ),
    };
    this.flow = {
      getFlows: () => request("/api/manager/flow/flow"),
      getFlowCardActions: () => request("/api/manager/flow/flowcardaction"),
      getAdvancedFlows: () => request("/api/manager/flow/advancedflow"),
    };
  }
}

function runtimeWith(
  responder: (call: FetchCall, index: number) => Patch039FetchResponseLike | Promise<Patch039FetchResponseLike>,
  HomeyAPIV3Local: Patch039HomeyApiRuntime["HomeyAPIV3Local"] = FakeHomeyAPIV3Local,
): { runtime: Patch039HomeyApiRuntime; calls: FetchCall[] } {
  const calls: FetchCall[] = [];
  return {
    calls,
    runtime: {
      HomeyAPIV3Local,
      Util: {
        async fetch(url, options = {}) {
          const call = { url, options: { ...options } };
          calls.push(call);
          return responder(call, calls.length - 1);
        },
      },
    },
  };
}

function runtimeWithPrivateGuardProbe(
  invoke: (instance: Record<string, (...args: any[]) => Promise<unknown>>) => Promise<unknown>,
): { runtime: Patch039HomeyApiRuntime; calls: FetchCall[] } {
  class GuardProbeHomeyAPIV3Local {
    id: string;
    devices: {
      getDevices(): Promise<unknown>;
      getCapabilityValue(input: { deviceId: string; capabilityId: string }): Promise<unknown>;
    };
    flow: {
      getFlows(): Promise<unknown>;
      getFlowCardActions(): Promise<unknown>;
      getAdvancedFlows(): Promise<unknown>;
    };

    constructor(options: Record<string, unknown>) {
      const properties = options.properties as Record<string, unknown>;
      this.id = String(properties.id);
      const run = () => invoke(this as unknown as Record<string, (...args: any[]) => Promise<unknown>>);
      this.devices = {
        getDevices: run,
        getCapabilityValue: run,
      };
      this.flow = {
        getFlows: run,
        getFlowCardActions: run,
        getAdvancedFlows: run,
      };
    }
  }

  return runtimeWith(() => pingSuccess(), GuardProbeHomeyAPIV3Local);
}

async function projectedGuardProbe(
  invoke: (instance: Record<string, (...args: any[]) => Promise<unknown>>) => Promise<unknown>,
): Promise<{ raw: Awaited<ReturnType<typeof createPatch039StrictLocalRawApi>>; calls: FetchCall[] }> {
  const { runtime, calls } = runtimeWithPrivateGuardProbe(invoke);
  const raw = await createPatch039StrictLocalRawApi({
    configuredAddress,
    expectedHomeyDigest: expectedDigest,
    getPersonalAccessToken: async () => privateToken,
    runtime,
  });
  return { raw, calls };
}

function pingSuccess(url = `${configuredAddress}/api/manager/system/ping`): Patch039FetchResponseLike {
  return response({
    url,
    headers: {
      "X-Homey-ID": rawHomeyId,
      "X-Homey-Version": "synthetic-version",
      "Content-Type": "application/json",
    },
    body: "{}",
  });
}

function tokenProvider(counter: { calls: number }): () => Promise<string> {
  return async () => {
    counter.calls += 1;
    return privateToken;
  };
}

test("invalid configured address fails before PAT retrieval and before fetch", async () => {
  const { runtime, calls } = runtimeWith(() => pingSuccess());
  const counter = { calls: 0 };
  await assert.rejects(createPatch039StrictLocalRawApi({
    configuredAddress: `${configuredAddress}/forbidden-path`,
    expectedHomeyDigest: expectedDigest,
    getPersonalAccessToken: tokenProvider(counter),
    runtime,
  }), /one origin without a path/);
  assert.equal(counter.calls, 0);
  assert.equal(calls.length, 0);
});

test("identity ping is unauthenticated and explicitly uses redirect manual", async () => {
  const { runtime, calls } = runtimeWith(() => pingSuccess());
  const identity = await probePatch039StrictIdentity({
    configuredAddress,
    expectedHomeyDigest: expectedDigest,
    util: runtime.Util,
  });
  assert.equal(identity.configuredOrigin, configuredAddress);
  assert.equal(identity.observedOrigin, configuredAddress);
  assert.equal(calls.length, 1);
  const options = calls[0]!.options;
  assert.equal(options.method, "GET");
  assert.equal(options.redirect, "manual");
  const headers = options.headers as Record<string, string>;
  assert.equal(Object.keys(headers).length, 0);
  assert.equal(headers.Authorization, undefined);
  assert.equal(headers.Cookie, undefined);
});

for (const status of [301, 302, 303, 307, 308]) {
  test(`redirect ${status} fails closed before PAT retrieval`, async () => {
    const { runtime, calls } = runtimeWith(() => response({
      status,
      headers: { Location: ["https:", "", "other.invalid", "target"].join("/") },
    }));
    const counter = { calls: 0 };
    await assert.rejects(createPatch039StrictLocalRawApi({
      configuredAddress,
      expectedHomeyDigest: expectedDigest,
      getPersonalAccessToken: tokenProvider(counter),
      runtime,
    }), /redirect response is forbidden/);
    assert.equal(counter.calls, 0);
    assert.equal(calls.length, 1);
  });
}

test("redirected response flag fails closed before PAT retrieval", async () => {
  const { runtime } = runtimeWith(() => response({
    redirected: true,
    headers: { "X-Homey-ID": rawHomeyId },
  }));
  const counter = { calls: 0 };
  await assert.rejects(createPatch039StrictLocalRawApi({
    configuredAddress,
    expectedHomeyDigest: expectedDigest,
    getPersonalAccessToken: tokenProvider(counter),
    runtime,
  }), /redirected response is forbidden/);
  assert.equal(counter.calls, 0);
});

test("observed response origin mismatch fails before PAT retrieval", async () => {
  const { runtime } = runtimeWith(() => pingSuccess(["https:", "", "other.invalid", "api/manager/system/ping"].join("/")));
  const counter = { calls: 0 };
  await assert.rejects(createPatch039StrictLocalRawApi({
    configuredAddress,
    expectedHomeyDigest: expectedDigest,
    getPersonalAccessToken: tokenProvider(counter),
    runtime,
  }), /target substitution/);
  assert.equal(counter.calls, 0);
});

test("identity mismatch fails before PAT retrieval", async () => {
  const { runtime } = runtimeWith(() => response({
    headers: { "X-Homey-ID": ["other", "synthetic", "homey"].join("-") },
  }));
  const counter = { calls: 0 };
  await assert.rejects(createPatch039StrictLocalRawApi({
    configuredAddress,
    expectedHomeyDigest: expectedDigest,
    getPersonalAccessToken: tokenProvider(counter),
    runtime,
  }), /does not match/);
  assert.equal(counter.calls, 0);
});

test("successful identity gate makes exactly one transition to PAT retrieval", async () => {
  const { runtime } = runtimeWith(() => pingSuccess());
  const counter = { calls: 0 };
  const raw = await createPatch039StrictLocalRawApi({
    configuredAddress,
    expectedHomeyDigest: expectedDigest,
    getPersonalAccessToken: tokenProvider(counter),
    runtime,
  });
  assert.equal(raw.id, rawHomeyId);
  assert.equal(counter.calls, 1);
  assert.deepEqual(Object.keys(raw).sort(), ["devices", "flow", "id"]);
  assert.deepEqual(Object.keys(raw.devices).sort(), ["getCapabilityValue", "getDevices"]);
  assert.deepEqual(Object.keys(raw.flow).sort(), ["getAdvancedFlows", "getFlowCardActions", "getFlows"]);
  assert.equal(Object.hasOwn(raw, "setCapabilityValue"), false);
});

test("authenticated manager read is GET-only, bearer-authenticated, manual-redirect, and socket-fallback only", async () => {
  const { runtime, calls } = runtimeWith((_call, index) => {
    if (index === 0) return pingSuccess();
    return response({
      url: `${configuredAddress}/api/manager/devices/device`,
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ a: { id: "synthetic-device" } }),
    });
  });
  const counter = { calls: 0 };
  const raw = await createPatch039StrictLocalRawApi({
    configuredAddress,
    expectedHomeyDigest: expectedDigest,
    getPersonalAccessToken: tokenProvider(counter),
    runtime,
  });
  const result = await raw.devices.getDevices();
  assert.deepEqual(result, { a: { id: "synthetic-device" } });
  assert.equal(calls.length, 2);
  const options = calls[1]!.options;
  assert.equal(options.method, "GET");
  assert.equal(options.redirect, "manual");
  assert.equal(Object.hasOwn(options, "body"), false);
  const headers = options.headers as Record<string, string>;
  assert.equal(headers["X-Homey-ID"], rawHomeyId);
  assert.equal(headers.Authorization, `Bearer ${privateToken}`);
  assert.equal(headers.Cookie, undefined);
  assert.equal(counter.calls, 1);
});

test("strict subclass factory is not exported from the production module", () => {
  assert.equal(Object.hasOwn(strictTransportModule, "createPatch039StrictHomeySubclass"), false);
});

test("strict runtime loader is not exported from the production module", () => {
  assert.equal(Object.hasOwn(strictTransportModule, "loadPatch039HomeyApiRuntime"), false);
});

test("socket __apiRequest fails locally with the exact manager fallback code", async () => {
  const { raw, calls } = await projectedGuardProbe((instance) => instance.__apiRequest!({}));
  const error = await raw.devices.getDevices().then(
    () => null,
    (value: unknown) => value as { code?: string },
  );
  assert.equal(error?.code, "ERR_SOCKET_SESSION_NOT_READY");
  assert.equal(calls.length, 1);
});

test("strict transport refuses non-GET, body-bearing, caller auth, target headers, and non-allowlisted manager paths before fetch", async () => {
  const rejectedCalls: Array<{ request: Record<string, unknown>; pattern: RegExp }> = [
    { request: { method: "POST", path: "/api/manager/devices/device" }, pattern: /GET only/ },
    { request: { method: "GET", path: "/api/manager/devices/device", body: {} }, pattern: /request bodies/ },
    {
      request: { method: "GET", path: "/api/manager/devices/device", headers: { Authorization: "forbidden" } },
      pattern: /caller-supplied credential or target/,
    },
    {
      request: { method: "GET", path: "/api/manager/devices/device", headers: { Host: "other.invalid" } },
      pattern: /caller-supplied credential or target/,
    },
    {
      request: { method: "GET", path: "/api/manager/devices/device", headers: { "X-Homey-ID": "caller-value" } },
      pattern: /caller-supplied credential or target/,
    },
    {
      request: { method: "GET", path: "/api/manager/devices/../../system/ping" },
      pattern: /non-manager path|outside the read-only boundary/,
    },
    {
      request: { method: "GET", path: "/api/manager/devices/%2e%2e/%2e%2e/system/ping" },
      pattern: /non-manager path|outside the read-only boundary/,
    },
    {
      request: { method: "GET", path: "/api/manager/system/ping" },
      pattern: /outside the read-only boundary/,
    },
  ];

  for (const { request, pattern } of rejectedCalls) {
    const { raw, calls } = await projectedGuardProbe((instance) => instance.call!(request));
    await assert.rejects(raw.devices.getDevices(), pattern);
    assert.equal(calls.length, 1);
  }
});

test("connect login subscribe and discovery are fail-closed", async () => {
  for (const method of ["connect", "login", "subscribe", "discoverBaseUrl"] as const) {
    const { raw, calls } = await projectedGuardProbe((instance) => instance[method]!());
    await assert.rejects(raw.devices.getDevices(), /forbidden/);
    assert.equal(calls.length, 1);
  }
});

test("authenticated redirect is rejected without a second target request", async () => {
  const { runtime, calls } = runtimeWith((_call, index) => {
    if (index === 0) return pingSuccess();
    return response({
      url: `${configuredAddress}/api/manager/flow/flow`,
      status: 302,
      headers: { Location: ["http:", "", "downgrade.invalid", "x"].join("/") },
    });
  });
  const raw = await createPatch039StrictLocalRawApi({
    configuredAddress,
    expectedHomeyDigest: expectedDigest,
    getPersonalAccessToken: async () => privateToken,
    runtime,
  });
  await assert.rejects(raw.flow.getFlows(), /redirect response is forbidden/);
  assert.equal(calls.length, 2);
});
