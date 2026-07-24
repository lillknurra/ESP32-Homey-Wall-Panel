import assert from "node:assert/strict";
import test from "node:test";
import { HomeyApiConnector } from "../src/homey-api-adapter.js";

test("live adapter cannot connect without explicit gate", async () => {
  const connector = new HomeyApiConnector("LOCAL", { liveEnabled: false });
  await assert.rejects(connector.connect(1000), /explicit --live/);
});

test("live adapter cannot connect without private config", async () => {
  const connector = new HomeyApiConnector("LOCAL", { liveEnabled: true });
  await assert.rejects(connector.connect(1000), /validated private configuration/);
});
