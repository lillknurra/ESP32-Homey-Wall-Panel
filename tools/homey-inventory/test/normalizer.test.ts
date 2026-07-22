import test from "node:test";
import assert from "node:assert/strict";
import { normalize } from "../src/normalizer.js";
import { SYNTHETIC_INVENTORY } from "../src/synthetic.js";

test("normalization is deterministic and contains no raw IDs", () => {
  const first = normalize(SYNTHETIC_INVENTORY, {}, {
    requested: "auto", selected: "LOCAL_SECURE", fallbackCount: 0,
  }, "2026-01-01T00:00:00.000Z");
  const second = normalize(SYNTHETIC_INVENTORY, {}, {
    requested: "auto", selected: "LOCAL_SECURE", fallbackCount: 0,
  }, "2026-01-01T00:00:00.000Z");
  assert.deepEqual(first.inventory, second.inventory);
  const serialized = JSON.stringify(first.inventory);
  for (const rawId of first.rawIds) assert.equal(serialized.includes(rawId), false);
});
