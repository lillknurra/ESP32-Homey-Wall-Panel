import assert from "node:assert/strict";
import test from "node:test";
import { createSanitizedLiveEvidence } from "../src/live-evidence.js";

test("live evidence accepts strict candidate and read operations", () => {
  const evidence = createSanitizedLiveEvidence({
    selected_candidate: "LOCAL_SECURE",
    operations: ["platform_metadata.read", "devices.read"],
  });
  assert.equal(evidence.selected_candidate, "LOCAL_SECURE");
  assert.deepEqual(evidence.operations, ["platform_metadata.read", "devices.read"]);
  assert.equal(evidence.credentials_present, false);
  assert.equal(evidence.raw_identifiers_present, false);
  assert.equal(evidence.raw_responses_present, false);
});

test("live evidence accepts null candidate before selection", () => {
  const evidence = createSanitizedLiveEvidence({
    selected_candidate: null,
    operations: [],
  });
  assert.equal(evidence.selected_candidate, null);
});

test("live evidence rejects arbitrary candidate strings at runtime", () => {
  assert.throws(
    () => createSanitizedLiveEvidence({
      selected_candidate: ["https", "://", "example.invalid", "/raw-id"].join("") as never,
      operations: [],
    }),
    /Invalid sanitized evidence candidate/,
  );
});

test("live evidence rejects non-allowlisted operations at runtime", () => {
  assert.throws(
    () => createSanitizedLiveEvidence({
      selected_candidate: "LOCAL",
      operations: ["setCapabilityValue" as never],
    }),
    /Invalid sanitized evidence operation/,
  );
});
