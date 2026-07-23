import assert from "node:assert/strict";
import test from "node:test";
import { SanitizedCallLedger } from "../src/call-ledger.js";
import { AllowlistedLiveCollector } from "../src/live-collector.js";

test("collector records only explicit read operations", async () => {
  const ledger = new SanitizedCallLedger();
  const collector = new AllowlistedLiveCollector({
    async readPlatformMetadata() { return { name: "Homey", version: "test" }; },
    async readZones() { return []; },
    async readDevices() { return []; },
    async readFlows() { return []; },
    async readAdvancedFlows() { return []; },
    async readMoods() { return []; },
  }, ledger);

  const inventory = await collector.collect();
  assert.equal(inventory.devices.length, 0);
  assert.deepEqual(ledger.entries(), [
    "platform_metadata.read",
    "zones.read",
    "devices.read",
    "flows.read",
    "advanced_flows.read",
    "moods.read",
  ]);
});
