import assert from "node:assert/strict";
import test from "node:test";
import { assertCompatibleReadSurface } from "../src/homey-api-compat.js";

test("accepts the mocked pinned read surface", () => {
  const surface = { platform: { getSystemInfo: async () => ({}) }, zones: { getZones: async () => ({}) },
    devices: { getDevices: async () => ({}) }, flow: { getFlows: async () => ({}), getAdvancedFlows: async () => ({}) },
    moods: { getMoods: async () => ({}) } };
  assert.doesNotThrow(() => assertCompatibleReadSurface(surface));
});

test("rejects missing read methods", () => assert.throws(() => assertCompatibleReadSurface({}), /incompatible/));
