import assert from "node:assert/strict";
import { createHash } from "node:crypto";
import test from "node:test";
import {
  createPatch039LocalReadonlyClient,
  PATCH039_HOMEY_API_SOURCE_CONTRACT,
  verifyPatch039SelectedHomey,
  wrapVerifiedPatch039HomeyApi,
} from "../src/awning-homey-api-adapter.js";
import { PATCH039_FORBIDDEN_HOMEY_METHOD_NAMES } from "../src/awning-readonly-client.js";

const homeyId = "synthetic-homey-id";
const digest = createHash("sha256").update(homeyId).digest("hex");

function fakeApi(onCapabilityValue?: (input: { deviceId: string; capabilityId: string }) => void) {
  return {
    id: homeyId,
    devices: {
      async getDevices() {
        return {
          a: {
            id: "synthetic-device-a",
            capabilities: ["needs_value", "has_value"],
            capabilitiesObj: {
              needs_value: { type: "boolean", setable: false },
              has_value: { type: "boolean", setable: false, value: true },
            },
          },
          b: { id: "synthetic-device-b", capabilities: [], capabilitiesObj: {} },
        };
      },
      async getCapabilityValue(input: { deviceId: string; capabilityId: string }) {
        onCapabilityValue?.(input);
        return false;
      },
    },
    flow: {
      async getFlows() { return { f: { id: "synthetic-flow" } }; },
      async getFlowCardActions() { return { c: { id: "synthetic-card" } }; },
      async getAdvancedFlows() { return {}; },
    },
    setCapabilityValue() { throw new Error("must never be exposed"); },
  };
}

test("source contract is pinned to exact 3.19.1 identity assumptions", () => {
  assert.equal(PATCH039_HOMEY_API_SOURCE_CONTRACT.package_version, "3.19.1");
  assert.equal(PATCH039_HOMEY_API_SOURCE_CONTRACT.identity_header, "X-Homey-ID");
  assert.equal(PATCH039_HOMEY_API_SOURCE_CONTRACT.credential_kind, "personal_access_token");
  assert.equal(PATCH039_HOMEY_API_SOURCE_CONTRACT.candidate_fallback, false);
});

test("selected Homey identity gate accepts exact digest and refuses missing or wrong identity", () => {
  assert.doesNotThrow(() => verifyPatch039SelectedHomey(homeyId, digest));
  assert.throws(() => verifyPatch039SelectedHomey(undefined, digest), /identity is unavailable/);
  assert.throws(() => verifyPatch039SelectedHomey(homeyId, "d".repeat(64)), /does not match/);
});

test("raw Homey API is encapsulated and only allowlisted reads are exposed", async () => {
  const client = wrapVerifiedPatch039HomeyApi(fakeApi(), digest);
  const exposed = Object.keys(client).sort();
  assert.deepEqual(exposed, [
    "getAdvancedFlows",
    "getCapabilityValue",
    "getDevices",
    "getFlowCardActions",
    "getFlows",
    "ledger",
    "sessionEvidence",
  ].sort());
  for (const forbidden of PATCH039_FORBIDDEN_HOMEY_METHOD_NAMES) {
    assert.equal(Object.hasOwn(client, forbidden), false);
  }
  assert.equal(client.sessionEvidence().selected_homey_verified, true);
  assert.equal(client.sessionEvidence().authentication_succeeded, false);
  assert.equal((await client.getDevices()).length, 2);
  assert.equal(client.sessionEvidence().authentication_succeeded, true);
  await client.getFlows();
  await client.getFlowCardActions();
  await client.getAdvancedFlows();
  assert.deepEqual(client.ledger.entries().map((entry) => entry.operation), [
    "devices.read",
    "flows.read",
    "flow_card_actions.read",
    "advanced_flows.read",
  ]);
});

test("conditional capability-value fallback refuses calls before fresh getDevices", async () => {
  let rawCalls = 0;
  const client = wrapVerifiedPatch039HomeyApi(fakeApi(() => { rawCalls += 1; }), digest);
  await assert.rejects(
    client.getCapabilityValue("synthetic-device-a", "needs_value"),
    /requires a successful fresh getDevices read/,
  );
  assert.equal(rawCalls, 0);
  assert.deepEqual(client.ledger.entries(), []);
});

test("conditional capability-value fallback refuses unobserved pair and already-present current value before Homey call", async () => {
  let rawCalls = 0;
  const client = wrapVerifiedPatch039HomeyApi(fakeApi(() => { rawCalls += 1; }), digest);
  await client.getDevices();
  await assert.rejects(
    client.getCapabilityValue("synthetic-device-a", "unobserved"),
    /pair was not observed as missing current-value evidence/,
  );
  await assert.rejects(
    client.getCapabilityValue("synthetic-device-a", "has_value"),
    /pair was not observed as missing current-value evidence/,
  );
  assert.equal(rawCalls, 0);
  assert.deepEqual(client.ledger.entries().map((entry) => entry.operation), ["devices.read"]);
});

test("conditional capability-value fallback allows exactly one observed missing-value pair per fresh device inventory", async () => {
  let rawCalls = 0;
  let observedInput: { deviceId: string; capabilityId: string } | null = null;
  const client = wrapVerifiedPatch039HomeyApi(fakeApi((input) => {
    rawCalls += 1;
    observedInput = input;
  }), digest);
  await client.getDevices();
  assert.equal(await client.getCapabilityValue("synthetic-device-a", "needs_value"), false);
  assert.deepEqual(observedInput, { deviceId: "synthetic-device-a", capabilityId: "needs_value" });
  assert.equal(rawCalls, 1);
  await assert.rejects(
    client.getCapabilityValue("synthetic-device-a", "needs_value"),
    /pair was not observed as missing current-value evidence/,
  );
  assert.equal(rawCalls, 1);
  assert.deepEqual(client.ledger.entries().map((entry) => entry.operation), [
    "devices.read",
    "capability_value.read",
  ]);
  const serializedLedger = JSON.stringify(client.ledger.entries());
  assert.equal(serializedLedger.includes("synthetic-device-a"), false);
  assert.equal(serializedLedger.includes("needs_value"), false);
});

test("a fresh device inventory re-establishes only currently missing capability-value fallbacks", async () => {
  let rawCalls = 0;
  const client = wrapVerifiedPatch039HomeyApi(fakeApi(() => { rawCalls += 1; }), digest);
  await client.getDevices();
  await client.getCapabilityValue("synthetic-device-a", "needs_value");
  await client.getDevices();
  await client.getCapabilityValue("synthetic-device-a", "needs_value");
  assert.equal(rawCalls, 2);
});

test("local factory receives PAT only after explicit target policy and has no candidate fallback", async () => {
  const address = ["https:", "", "synthetic-homey.invalid"].join("/");
  let calls = 0;
  const client = await createPatch039LocalReadonlyClient({
    configuredAddress: address,
    observedAddress: address,
    personalAccessToken: "synthetic-pat",
    expectedHomeyDigest: digest,
    factory: async (options) => {
      calls += 1;
      assert.equal(options.address, address);
      assert.equal(options.token, "synthetic-pat");
      return fakeApi();
    },
  });
  assert.equal(calls, 1);
  assert.equal(client.sessionEvidence().selected_homey_verified, true);
});
