import test from "node:test";
import assert from "node:assert/strict";
import { selectConnection } from "../src/connection-selector.js";
import { CandidateError } from "../src/errors.js";
import type { CandidateConnector, ReadonlyHomeyClient } from "../src/readonly-client.js";
import type { CandidateKind, RawInventory } from "../src/model.js";

const empty: RawInventory = {
  platform: { name: null, version: null },
  zones: [], devices: [], flows: [], advancedFlows: [], moods: [],
};
const client: ReadonlyHomeyClient = { async collect() { return empty; } };

class Connector implements CandidateConnector {
  constructor(
    public readonly kind: CandidateKind,
    private readonly failure?: CandidateError,
  ) {}
  async connect(): Promise<ReadonlyHomeyClient> {
    if (this.failure) throw this.failure;
    return client;
  }
}

test("auto falls back only for reachability failures", async () => {
  const result = await selectConnection("auto", [
    new Connector("LOCAL_SECURE", new CandidateError("REACHABILITY", "offline")),
    new Connector("LOCAL"),
  ], 1000);
  assert.equal(result.selected, "LOCAL");
  assert.equal(result.fallbackCount, 1);
});

test("authentication failures stop fallback", async () => {
  await assert.rejects(
    selectConnection("auto", [
      new Connector("LOCAL_SECURE", new CandidateError("AUTHENTICATION", "denied")),
      new Connector("CLOUD"),
    ], 1000),
    /denied/,
  );
});
