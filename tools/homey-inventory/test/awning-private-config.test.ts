import assert from "node:assert/strict";
import { chmod, mkdtemp, writeFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";
import test from "node:test";
import {
  assertPatch039ObservedAddressPolicy,
  loadPatch039PrivateConfig,
  normalizePatch039HomeyAddress,
} from "../src/awning-private-config.js";

const privateAddress = () => ["https:", "", "synthetic-homey.invalid"].join("/");

async function writeConfig(path: string, overrides: Record<string, unknown> = {}) {
  const value = {
    schema_version: 1,
    connection: "local",
    credential_kind: "personal_access_token",
    credential_provider: "macos-keychain",
    keychain_service: "synthetic-service",
    keychain_account: "synthetic-account",
    homey_address: privateAddress(),
    expected_homey_id_sha256: "a".repeat(64),
    ...overrides,
  };
  await writeFile(path, JSON.stringify(value), { mode: 0o600 });
  await chmod(path, 0o600);
}

test("Patch039 private config is local PAT Keychain only and outside repo", async () => {
  const repo = await mkdtemp(join(tmpdir(), "patch039-repo-"));
  const outside = await mkdtemp(join(tmpdir(), "patch039-private-"));
  const path = join(outside, "config.json");
  await writeConfig(path);
  const config = await loadPatch039PrivateConfig(path, repo);
  assert.equal(config.connection, "local");
  assert.equal(config.credential_kind, "personal_access_token");
  assert.equal(config.credential_provider, "macos-keychain");
  assert.equal(config.homey_address, privateAddress());
});

test("Patch039 private config refuses environment credentials and unknown fields", async () => {
  const repo = await mkdtemp(join(tmpdir(), "patch039-repo-"));
  const outside = await mkdtemp(join(tmpdir(), "patch039-private-"));
  const envPath = join(outside, "env.json");
  await writeConfig(envPath, { credential_provider: "environment" });
  await assert.rejects(loadPatch039PrivateConfig(envPath, repo), /macos-keychain/);
  const extraPath = join(outside, "extra.json");
  await writeConfig(extraPath, { extra: "forbidden" });
  await assert.rejects(loadPatch039PrivateConfig(extraPath, repo), /unknown fields/);
});

test("Patch039 private config refuses permissive permissions", async () => {
  const repo = await mkdtemp(join(tmpdir(), "patch039-repo-"));
  const outside = await mkdtemp(join(tmpdir(), "patch039-private-"));
  const path = join(outside, "config.json");
  await writeConfig(path);
  await chmod(path, 0o644);
  await assert.rejects(loadPatch039PrivateConfig(path, repo), /permissions/);
});

test("Patch039 target policy refuses redirect and secure downgrade", () => {
  const https = privateAddress();
  assert.equal(normalizePatch039HomeyAddress(`${https}/`), https);
  assert.doesNotThrow(() => assertPatch039ObservedAddressPolicy(https, https));
  const other = ["https:", "", "other.invalid"].join("/");
  assert.throws(() => assertPatch039ObservedAddressPolicy(https, other), /substitution/);
  const insecure = ["http:", "", "synthetic-homey.invalid"].join("/");
  assert.throws(() => assertPatch039ObservedAddressPolicy(https, insecure), /downgrade/);
});
