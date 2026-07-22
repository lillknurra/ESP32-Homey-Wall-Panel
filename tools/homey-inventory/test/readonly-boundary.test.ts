import test from "node:test";
import assert from "node:assert/strict";
import { FORBIDDEN_MUTATION_NAMES } from "../src/readonly-client.js";
import * as readonlyModule from "../src/readonly-client.js";

test("read-only module exposes no mutation function", () => {
  for (const name of FORBIDDEN_MUTATION_NAMES) {
    assert.equal(Object.hasOwn(readonlyModule, name), false);
  }
});
