import assert from "node:assert/strict";
import { mkdtemp, writeFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";
import test from "node:test";
import { loadPrivateLiveConfig } from "../src/private-config.js";

test("private config must remain outside repository", async () => {
  const repo = await mkdtemp(join(tmpdir(), "homey-repo-"));
  const path = join(repo, "private.json");
  await writeFile(path, JSON.stringify({
    connection: "local",
    credential_provider: "environment",
    environment_variable: "HOMEY_TOKEN",
  }));
  await assert.rejects(loadPrivateLiveConfig(path, repo), /outside the repository/);
});

test("private config validates provider contract", async () => {
  const repo = await mkdtemp(join(tmpdir(), "homey-repo-"));
  const outside = await mkdtemp(join(tmpdir(), "homey-private-"));
  const path = join(outside, "private.json");
  await writeFile(path, JSON.stringify({
    connection: "local",
    credential_provider: "macos-keychain",
    keychain_service: "ESP32-Homey-Wall-Panel",
    keychain_account: "homey-pat",
  }));
  const config = await loadPrivateLiveConfig(path, repo);
  assert.equal(config.credential_provider, "macos-keychain");
});
