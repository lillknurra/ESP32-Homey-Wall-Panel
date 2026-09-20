import assert from "node:assert/strict";
import test from "node:test";
import { chmod, mkdtemp } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { buildPrivateOperatorCandidates, createPrivateAwningMappingFromSelection } from "../src/awning-operator-selection.js";
import {
  assertMappedDevicesFresh,
  assertMappingMatchesSelectedHomey,
  validatePrivateAwningMapping,
  loadPrivateAwningMapping,
  savePrivateAwningMapping,
} from "../src/awning-private-mapping.js";

const digest = "b".repeat(64);

const devices = [
  { id: "synthetic-device-a", name: "Synthetic A", driverId: "synthetic-driver", capabilities: ["up", "stop", "down"] },
  { id: "synthetic-device-b", name: "Synthetic B", driverId: "synthetic-driver", capabilities: ["up", "stop", "down"] },
  { id: "synthetic-device-c", name: "Synthetic C", driverId: "synthetic-driver", capabilities: ["up", "stop", "down"] },
];

test("operator candidate list exposes sanitized aliases but keeps raw correlation private", () => {
  const result = buildPrivateOperatorCandidates(devices, {});
  assert.equal(result.candidates.length, 3);
  const serialized = JSON.stringify(result.candidates);
  for (const device of devices) assert.equal(serialized.includes(device.id), false);
  assert.equal(result.privateCorrelation.size, 3);
  assert.match(result.candidates[0]!.device_alias, /^device_[0-9a-f]{12}$/);
});

test("explicit operator selection creates exactly three unique private mappings", () => {
  const result = buildPrivateOperatorCandidates(devices, {});
  const aliases = result.candidates.map((item) => item.device_alias);
  const mapping = createPrivateAwningMappingFromSelection({
    selections: { awning_1: aliases[0]!, awning_2: aliases[1]!, awning_3: aliases[2]! },
    privateCorrelation: result.privateCorrelation,
    selectedHomeyDigest: digest,
    generation: 1,
  });
  const validated = validatePrivateAwningMapping(mapping);
  assert.equal(validated.purpose, "read_only_awning_evidence");
  assertMappingMatchesSelectedHomey(validated, digest);
  assertMappedDevicesFresh(validated, devices.map((item) => item.id));
});

test("duplicate selection, selected-Homey mismatch, and stale mapped devices fail closed", () => {
  const result = buildPrivateOperatorCandidates(devices, {});
  const aliases = result.candidates.map((item) => item.device_alias);
  assert.throws(() => createPrivateAwningMappingFromSelection({
    selections: { awning_1: aliases[0]!, awning_2: aliases[0]!, awning_3: aliases[2]! },
    privateCorrelation: result.privateCorrelation,
    selectedHomeyDigest: digest,
    generation: 1,
  }), /three unique devices/);

  const mapping = createPrivateAwningMappingFromSelection({
    selections: { awning_1: aliases[0]!, awning_2: aliases[1]!, awning_3: aliases[2]! },
    privateCorrelation: result.privateCorrelation,
    selectedHomeyDigest: digest,
    generation: 1,
  });
  assert.throws(() => assertMappingMatchesSelectedHomey(mapping, "c".repeat(64)), /mismatch/);
  assert.throws(() => assertMappedDevicesFresh(mapping, [devices[0]!.id, devices[1]!.id]), /stale or ambiguous/);
  assert.throws(() => assertMappedDevicesFresh(mapping, [devices[0]!.id, devices[0]!.id, devices[1]!.id, devices[2]!.id]), /stale or ambiguous/);
});


test("private mapping persistence is outside Git, restrictive, atomic, and readback validated", async () => {
  const repo = await mkdtemp(join(tmpdir(), "patch039-map-repo-"));
  const outside = await mkdtemp(join(tmpdir(), "patch039-map-private-"));
  const result = buildPrivateOperatorCandidates(devices, {});
  const aliases = result.candidates.map((item) => item.device_alias);
  const mapping = createPrivateAwningMappingFromSelection({
    selections: { awning_1: aliases[0]!, awning_2: aliases[1]!, awning_3: aliases[2]! },
    privateCorrelation: result.privateCorrelation,
    selectedHomeyDigest: digest,
    generation: 7,
  });
  const path = join(outside, "awning-map.json");
  await savePrivateAwningMapping(path, repo, mapping);
  assert.deepEqual(await loadPrivateAwningMapping(path, repo), mapping);
  await chmod(path, 0o644);
  await assert.rejects(savePrivateAwningMapping(path, repo, mapping), /permissions are too broad/);
});


test("Patch039 refuses non-canonical or colliding sanitized aliases", () => {
  assert.throws(() => buildPrivateOperatorCandidates(devices, {
    "device:synthetic-device-a": "device_bad",
  }), /non-canonical alias/);
  assert.throws(() => buildPrivateOperatorCandidates(devices, {
    "device:synthetic-device-a": "device_aaaaaaaaaaaa",
    "device:synthetic-device-b": "device_aaaaaaaaaaaa",
  }), /alias collision/);
});
