import assert from "node:assert/strict";
import { chmod, mkdtemp, mkdir, writeFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";
import test from "node:test";
import { runPrivatePreflight } from "../src/preflight.js";

const envConfig = {
  connection: "local" as const,
  credential_provider: "environment" as const,
  environment_variable: "HOMEY_TOKEN",
};

test("preflight accepts restrictive private file outside repository", async () => {
  const repo = await mkdtemp(join(tmpdir(), "homey-repo-"));
  const outside = await mkdtemp(join(tmpdir(), "homey-private-"));
  const path = join(outside, "private.json");
  await writeFile(path, "{}", { mode: 0o600 });
  await chmod(path, 0o600);

  const report = await runPrivatePreflight(path, repo, envConfig, "darwin");
  assert.equal(report.config_outside_repository, true);
  assert.equal(report.restrictive_permissions, true);
});

test("preflight rejects private config inside repository", async () => {
  const repo = await mkdtemp(join(tmpdir(), "homey-repo-"));
  const path = join(repo, "private.json");
  await writeFile(path, "{}", { mode: 0o600 });
  await chmod(path, 0o600);

  await assert.rejects(
    runPrivatePreflight(path, repo, envConfig, "darwin"),
    /outside the repository/,
  );
});

test("preflight handles path prefixes correctly", async () => {
  const parent = await mkdtemp(join(tmpdir(), "homey-prefix-"));
  const repo = join(parent, "repo");
  const repoOther = join(parent, "repo-other");
  await mkdir(repo);
  await mkdir(repoOther);
  const path = join(repoOther, "private.json");
  await writeFile(path, "{}", { mode: 0o600 });
  await chmod(path, 0o600);

  const report = await runPrivatePreflight(path, repo, envConfig, "darwin");
  assert.equal(report.config_outside_repository, true);
});

test("preflight rejects permissive permissions", async () => {
  const repo = await mkdtemp(join(tmpdir(), "homey-repo-"));
  const outside = await mkdtemp(join(tmpdir(), "homey-private-"));
  const path = join(outside, "private.json");
  await writeFile(path, "{}", { mode: 0o644 });
  await chmod(path, 0o644);

  await assert.rejects(
    runPrivatePreflight(path, repo, envConfig, "darwin"),
    /permissions/,
  );
});
