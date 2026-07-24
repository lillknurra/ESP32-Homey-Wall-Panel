import assert from "node:assert/strict";
import test from "node:test";
import { classifyLiveFailure } from "../src/failure-classifier.js";

test("classifies required live failure classes", () => {
  assert.equal(classifyLiveFailure({ code: "ENOTFOUND" }), "DISCOVERY");
  assert.equal(classifyLiveFailure({ code: "ETIMEDOUT" }), "REACHABILITY");
  assert.equal(classifyLiveFailure({ statusCode: 401 }), "AUTHENTICATION");
  assert.equal(classifyLiveFailure({ statusCode: 403 }), "AUTHORIZATION");
  assert.equal(classifyLiveFailure({ code: "CERT_HAS_EXPIRED" }), "TLS");
  assert.equal(classifyLiveFailure(new Error("missing method")), "API_INCOMPATIBILITY");
  assert.equal(classifyLiveFailure(new Error("malformed response")), "MALFORMED_RESPONSE");
  assert.equal(classifyLiveFailure(new Error("bad option")), "CONFIGURATION");
});
