import assert from "node:assert/strict";
import { chmod, mkdtemp, writeFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";
import test from "node:test";
import { runControlledPreflight } from "../src/preflight-runner.js";

test("controlled preflight uses existence check without reading credential", async () => {
  const repo = await mkdtemp(join(tmpdir(), "homey-repo-"));
  const outside = await mkdtemp(join(tmpdir(), "homey-private-"));
  const configPath = join(outside, "private.json");
  await writeFile(configPath, JSON.stringify({
    connection: "auto",
    credential_provider: "macos-keychain",
    keychain_service: "test-service",
    keychain_account: "test-account",
  }), { mode: 0o600 });
  await chmod(configPath, 0o600);
  let called = 0;
  const evidence = await runControlledPreflight(configPath, repo, {
    checkEntry: async () => { called += 1; return true; },
  });
  assert.equal(called, 1);
  assert.equal(evidence.credential_entry_exists, true);
  assert.equal(evidence.credential_value_read, false);
  assert.equal(evidence.homey_client_constructed, false);
  assert.equal(evidence.discovery_attempted, false);
  assert.equal(evidence.authentication_attempted, false);
  assert.equal(evidence.network_access_attempted, false);
});

test("controlled preflight refuses absent credential entry without network", async () => {
  const repo = await mkdtemp(join(tmpdir(), "homey-repo-"));
  const outside = await mkdtemp(join(tmpdir(), "homey-private-"));
  const configPath = join(outside, "private.json");
  await writeFile(configPath, JSON.stringify({
    connection: "auto",
    credential_provider: "macos-keychain",
    keychain_service: "test-service",
    keychain_account: "test-account",
  }), { mode: 0o600 });
  await chmod(configPath, 0o600);
  const evidence = await runControlledPreflight(configPath, repo, { checkEntry: async () => false });
  assert.equal(evidence.status, "REFUSED");
  assert.equal(evidence.network_access_attempted, false);
});


test("controlled preflight refuses environment provider without reading its value", async () => {
  const repo = await mkdtemp(join(tmpdir(), "homey-repo-"));
  const outside = await mkdtemp(join(tmpdir(), "homey-private-"));
  const configPath = join(outside, "private.json");
  const variableName = "PATCH_005_TEST_ENVIRONMENT_VALUE";
  const sentinel = ["do", "-not", "-read", "-this"].join("");
  process.env[variableName] = sentinel;
  await writeFile(configPath, JSON.stringify({
    connection: "auto",
    credential_provider: "environment",
    environment_variable: variableName,
  }), { mode: 0o600 });
  await chmod(configPath, 0o600);
  try {
    const evidence = await runControlledPreflight(configPath, repo, {
      checkEntry: async () => { throw new Error("environment existence check must not run"); },
    });
    assert.equal(evidence.status, "REFUSED");
    assert.equal(evidence.provider, "environment");
    assert.equal(evidence.provider_available, false);
    assert.equal(evidence.credential_entry_exists, false);
    assert.equal(evidence.credential_value_read, false);
    assert.equal(JSON.stringify(evidence).includes(sentinel), false);
  } finally {
    delete process.env[variableName];
  }
});
