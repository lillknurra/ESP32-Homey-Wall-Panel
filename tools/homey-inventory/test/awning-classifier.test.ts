import assert from "node:assert/strict";
import test from "node:test";
import { classifyAwningCandidates } from "../src/awning-candidate-classifier.js";
import { normalizeAwningEvidenceSources } from "../src/awning-evidence-normalizer.js";
import type { PrivateAwningMapping } from "../src/awning-private-mapping.js";

const mapping: PrivateAwningMapping = {
  schema_version: 1,
  purpose: "read_only_awning_evidence",
  generation: 1,
  selected_homey_id_sha256: "e".repeat(64),
  mappings: {
    awning_1: "synthetic-device-a",
    awning_2: "synthetic-device-b",
    awning_3: "synthetic-device-c",
  },
};

const baseDevices = [
  { id: "synthetic-device-a", available: true, class: "blinds", driverId: "synthetic-driver-a", capabilitiesObj: { position: { value: 0.25, lastUpdated: 1 } } },
  { id: "synthetic-device-b", available: true, class: "blinds", driverId: "synthetic-driver-b", capabilitiesObj: { position: { value: 0.50 } } },
  { id: "synthetic-device-c", available: true, class: "blinds", driverId: "synthetic-driver-c", capabilitiesObj: { position: { value: "private-string-value" } } },
  { id: "synthetic-unrelated-device", available: true, class: "other", capabilitiesObj: {} },
];

const descriptors = [
  { id: "synthetic-card-open", title: "Open awning", ownerId: "synthetic-owner" },
  { id: "synthetic-card-stop", title: "Stop awning", ownerId: "synthetic-owner" },
  { id: "synthetic-card-close", title: "Close awning", ownerId: "synthetic-owner" },
];

function normalize(flows: unknown[] = [], advancedFlows: unknown[] = [], devices: unknown[] = baseDevices) {
  return normalizeAwningEvidenceSources({
    devices,
    flows,
    flowCardActions: descriptors,
    advancedFlows,
  }, mapping, {});
}

function classify(normalized: ReturnType<typeof normalizeAwningEvidenceSources>) {
  return classifyAwningCandidates(normalized.observations, normalized.negativeEvidenceComplete);
}

function enumCommand(extra: Record<string, unknown> = {}) {
  return {
    type: "enum",
    setable: true,
    values: [
      { id: "raise", title: "Open" },
      { id: "halt", title: "Stop" },
      { id: "lower", title: "Close" },
    ],
    value: "halt",
    ...extra,
  };
}

test("single targeted Flow action becomes a read-only candidate and complete missing STOP is NO_CANDIDATE", () => {
  const normalized = normalize([
    { id: "synthetic-flow-open", enabled: true, triggerable: true, actions: [
      { id: "synthetic-card-open", args: { device: "synthetic-device-a" } },
    ] },
  ]);
  const classified = classify(normalized);
  assert.equal(normalized.negativeEvidenceComplete, true);
  assert.equal(classified.awning_1.OPEN.classification, "CANDIDATE_FLOW");
  assert.equal(classified.awning_1.STOP.classification, "NO_CANDIDATE");
  const serialized = JSON.stringify({
    devices: normalized.devices,
    flows: normalized.flows,
    advancedFlows: normalized.advancedFlows,
    observations: normalized.observations,
  });
  assert.equal(serialized.includes("synthetic-device-a"), false);
  assert.equal(serialized.includes("synthetic-card-open"), false);
  assert.equal(serialized.includes("synthetic-owner"), false);
});

test("observed enum capability metadata can produce device-capability candidates without inferring numeric orientation", () => {
  const devices = structuredClone(baseDevices) as Array<Record<string, unknown>>;
  devices[0] = {
    id: "synthetic-device-a",
    available: true,
    class: "blinds",
    driverId: "synthetic-driver-a",
    capabilities: ["command", "position"],
    capabilitiesObj: {
      command: enumCommand(),
      position: { type: "number", setable: true, min: 0, max: 1, value: 0.4 },
    },
  };
  const normalized = normalize([], [], devices);
  const classified = classify(normalized);
  assert.equal(classified.awning_1.OPEN.classification, "CANDIDATE_DEVICE_CAPABILITY");
  assert.equal(classified.awning_1.STOP.classification, "CANDIDATE_DEVICE_CAPABILITY");
  assert.equal(classified.awning_1.CLOSE.classification, "CANDIDATE_DEVICE_CAPABILITY");
  const published = JSON.stringify(normalized.devices);
  assert.equal(published.includes("raise"), false);
  assert.equal(published.includes("halt"), false);
  assert.equal(published.includes("lower"), false);
});

test("capability semantic without verified command value stays INSUFFICIENT_EVIDENCE", () => {
  const devices = structuredClone(baseDevices) as Array<Record<string, unknown>>;
  devices[0] = {
    id: "synthetic-device-a",
    available: true,
    class: "blinds",
    driverId: "synthetic-driver-a",
    capabilitiesObj: {
      maybeStop: { title: "Stop awning", setable: true, type: "boolean", value: false },
    },
  };
  const classified = classify(normalize([], [], devices));
  assert.equal(classified.awning_1.STOP.classification, "INSUFFICIENT_EVIDENCE");
  assert.ok(classified.awning_1.STOP.blocking_reasons.includes("CAPABILITY_ACTION_VALUE_NOT_VERIFIED"));
});

test("unknown capability open-schema fields block negative NO_CANDIDATE claims", () => {
  const devices = structuredClone(baseDevices) as Array<Record<string, unknown>>;
  devices[0] = {
    id: "synthetic-device-a",
    available: true,
    class: "blinds",
    driverId: "synthetic-driver-a",
    capabilitiesObj: { position: { value: 0.2, mystery: "private-value" } },
  };
  const normalized = normalize([], [], devices);
  const classified = classify(normalized);
  assert.equal(normalized.negativeEvidenceComplete, false);
  assert.equal(classified.awning_1.STOP.classification, "INSUFFICIENT_EVIDENCE");
  assert.equal(JSON.stringify(normalized.devices).includes("private-value"), false);
});

test("capabilities list missing capabilitiesObj evidence blocks NO_CANDIDATE", () => {
  const devices = structuredClone(baseDevices) as Array<Record<string, unknown>>;
  devices[0] = {
    id: "synthetic-device-a",
    available: true,
    class: "blinds",
    capabilities: ["position", "unrepresented-control"],
    capabilitiesObj: { position: { value: 0.2 } },
  };
  const normalized = normalize([], [], devices);
  const classified = classify(normalized);
  assert.equal(normalized.negativeEvidenceComplete, false);
  assert.equal(classified.awning_1.OPEN.classification, "INSUFFICIENT_EVIDENCE");
  assert.notEqual(classified.awning_1.OPEN.classification, "NO_CANDIDATE");
});

test("malformed or duplicate capabilities list blocks positive promotion and negative completeness", () => {
  for (const capabilities of ["command", ["command", "command"]] as unknown[]) {
    const devices = structuredClone(baseDevices) as Array<Record<string, unknown>>;
    devices[0] = {
      id: "synthetic-device-a",
      available: true,
      class: "blinds",
      capabilities,
      capabilitiesObj: { command: enumCommand() },
    };
    const normalized = normalize([], [], devices);
    const classified = classify(normalized);
    assert.equal(normalized.negativeEvidenceComplete, false);
    assert.equal(classified.awning_1.OPEN.classification, "INSUFFICIENT_EVIDENCE");
    assert.ok(classified.awning_1.OPEN.blocking_reasons.includes("CAPABILITY_LIST_INCONSISTENT"));
  }
});

test("non-empty capability options block device-capability candidate promotion", () => {
  const devices = structuredClone(baseDevices) as Array<Record<string, unknown>>;
  devices[0] = {
    id: "synthetic-device-a",
    available: true,
    class: "blinds",
    capabilities: ["command"],
    capabilitiesObj: { command: enumCommand({ options: { unknownSemantic: "private-value" } }) },
  };
  const normalized = normalize([], [], devices);
  const classified = classify(normalized);
  assert.equal(classified.awning_1.OPEN.classification, "INSUFFICIENT_EVIDENCE");
  assert.ok(classified.awning_1.OPEN.blocking_reasons.includes("CAPABILITY_OPTIONS_UNINSPECTED"));
  assert.equal(JSON.stringify(normalized.devices).includes("private-value"), false);
});

test("Flow with unrelated device side effects is AMBIGUOUS", () => {
  const normalized = normalize([
    { id: "synthetic-flow-close", actions: [
      { id: "synthetic-card-close", args: { primary: "synthetic-device-a", secondary: "synthetic-unrelated-device" } },
    ] },
  ]);
  const classified = classify(normalized);
  assert.equal(classified.awning_1.CLOSE.classification, "AMBIGUOUS");
  assert.ok(classified.awning_1.CLOSE.blocking_reasons.includes("MULTI_DEVICE_OR_MULTI_ACTION_SIDE_EFFECT"));
});

test("unknown Flow fields block candidate promotion and negative-evidence completeness", () => {
  const normalized = normalize([
    { id: "synthetic-flow-stop", actions: [
      { id: "synthetic-card-stop", args: { device: "synthetic-device-a" }, unknownPayload: "private-value" },
    ] },
  ]);
  const classified = classify(normalized);
  assert.equal(normalized.negativeEvidenceComplete, false);
  assert.equal(classified.awning_1.STOP.classification, "INSUFFICIENT_EVIDENCE");
  assert.ok(classified.awning_1.STOP.blocking_reasons.includes("FLOW_ACTION_UNKNOWN_FIELDS"));
  assert.equal(JSON.stringify(normalized.flows).includes("private-value"), false);
});

test("nested or additional saved Flow args block descriptor-only promotion", () => {
  for (const args of [
    { device: "synthetic-device-a", mode: "private-open-mode" },
    { target: { device: "synthetic-device-a" } },
  ]) {
    const normalized = normalize([
      { id: "synthetic-flow-open", actions: [{ id: "synthetic-card-open", args }] },
    ]);
    const classified = classify(normalized);
    assert.equal(classified.awning_1.OPEN.classification, "INSUFFICIENT_EVIDENCE");
    assert.ok(classified.awning_1.OPEN.blocking_reasons.includes("FLOW_ARGS_UNVERIFIED"));
    const serialized = JSON.stringify(normalized.flows);
    assert.equal(serialized.includes("private-open-mode"), false);
    assert.deepEqual(Object.keys(normalized.flows[0]!.actions[0]!).sort(), [
      "card_alias", "owner_alias", "referenced_device_count", "target_roles", "unknown_field_names",
    ]);
  }
});

test("clean single-target saved Flow args plus descriptor semantic can promote candidate", () => {
  const normalized = normalize([
    { id: "synthetic-flow-close", actions: [
      { id: "synthetic-card-close", args: { device: "synthetic-device-b" } },
    ] },
  ]);
  assert.equal(classify(normalized).awning_2.CLOSE.classification, "CANDIDATE_FLOW");
});

test("Advanced Flow action card can become candidate while unknown fields fail closed", () => {
  const clean = normalize([], [
    { id: "synthetic-advanced-open", cards: {
      card1: { id: "synthetic-card-open", ownerUri: "synthetic-owner", type: "action", x: 1, y: 2, args: { device: "synthetic-device-b" } },
    } },
  ]);
  let classified = classify(clean);
  assert.equal(classified.awning_2.OPEN.classification, "CANDIDATE_ADVANCED_FLOW");
  assert.equal(JSON.stringify(clean.advancedFlows).includes("synthetic-owner"), false);

  const blocked = normalize([], [
    { id: "synthetic-advanced-stop", cards: {
      card1: { id: "synthetic-card-stop", type: "action", x: 1, y: 2, args: { device: "synthetic-device-b" }, topology: "private-value" },
    } },
  ]);
  classified = classify(blocked);
  assert.equal(classified.awning_2.STOP.classification, "INSUFFICIENT_EVIDENCE");
  assert.ok(classified.awning_2.STOP.blocking_reasons.includes("ADVANCED_FLOW_CARD_UNKNOWN_FIELDS"));
});

test("nested or additional Advanced Flow args block descriptor-only promotion", () => {
  const normalized = normalize([], [
    { id: "synthetic-advanced-open", cards: {
      card1: {
        id: "synthetic-card-open",
        type: "action",
        x: 1,
        y: 2,
        args: { device: "synthetic-device-c", mode: { nested: "private-mode" } },
      },
    } },
  ]);
  const classified = classify(normalized);
  assert.equal(classified.awning_3.OPEN.classification, "INSUFFICIENT_EVIDENCE");
  assert.ok(classified.awning_3.OPEN.blocking_reasons.includes("ADVANCED_FLOW_ARGS_UNVERIFIED"));
  const serialized = JSON.stringify(normalized.advancedFlows);
  assert.equal(serialized.includes("private-mode"), false);
  assert.equal(serialized.includes("mode"), false);
});

test("competing OPEN and CLOSE candidates are AMBIGUOUS", () => {
  const normalized = normalize([
    { id: "synthetic-flow-open-a", actions: [{ id: "synthetic-card-open", args: { device: "synthetic-device-c" } }] },
    { id: "synthetic-flow-open-b", actions: [{ id: "synthetic-card-open", args: { device: "synthetic-device-c" } }] },
    { id: "synthetic-flow-close-a", actions: [{ id: "synthetic-card-close", args: { device: "synthetic-device-c" } }] },
    { id: "synthetic-flow-close-b", actions: [{ id: "synthetic-card-close", args: { device: "synthetic-device-c" } }] },
  ]);
  const classified = classify(normalized);
  assert.equal(classified.awning_3.OPEN.classification, "AMBIGUOUS");
  assert.equal(classified.awning_3.CLOSE.classification, "AMBIGUOUS");
});

test("unknown capability values never pass through as arbitrary strings", () => {
  const normalized = normalize();
  const cap = normalized.devices.find((device) => device.role === "awning_3")!.capabilities[0]!;
  assert.equal(cap.value_type, "string");
  assert.equal(cap.value, null);
  const serialized = JSON.stringify(normalized.devices);
  assert.equal(serialized.includes("private-string-value"), false);
});
