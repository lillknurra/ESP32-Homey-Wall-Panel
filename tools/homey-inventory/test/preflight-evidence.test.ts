import assert from "node:assert/strict";
import test from "node:test";
import { createSanitizedPreflightEvidence } from "../src/preflight-evidence.js";

test("controlled preflight evidence contains only sanitized boundary fields", () => {
  const evidence = createSanitizedPreflightEvidence("macos-keychain", {
    config_valid: true,
    config_outside_repository: true,
    permissions_restrictive: true,
    provider_available: true,
    credential_entry_exists: true,
  });
  assert.equal(evidence.status, "PASS");
  assert.equal(evidence.credential_value_read, false);
  assert.equal(evidence.homey_client_constructed, false);
  assert.equal(evidence.network_access_attempted, false);
  const encoded = JSON.stringify(evidence);
  for (const forbidden of ["token", "secret", "path", "service", "account", "address", "url", "homey_selector"]) {
    assert.equal(encoded.toLowerCase().includes(forbidden), false);
  }
});


test("environment refusal evidence remains sanitized", () => {
  const evidence = createSanitizedPreflightEvidence("environment", {
    config_valid: true,
    config_outside_repository: true,
    permissions_restrictive: true,
    provider_available: false,
    credential_entry_exists: false,
  });
  assert.equal(evidence.status, "REFUSED");
  assert.equal(evidence.credential_value_read, false);
  assert.deepEqual(Object.keys(evidence).sort(), [
    "authentication_attempted", "config_outside_repository", "config_valid",
    "credential_entry_exists", "credential_value_read", "discovery_attempted",
    "homey_client_constructed", "mode", "network_access_attempted",
    "permissions_restrictive", "provider", "provider_available", "status",
  ].sort());
});
