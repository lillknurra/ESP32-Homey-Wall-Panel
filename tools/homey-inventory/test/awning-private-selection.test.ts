import assert from "node:assert/strict";
import { chmod, mkdtemp, writeFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";
import test from "node:test";
import {
  loadPatch042PrivateSelection,
  validatePatch042PrivateSelection,
} from "../src/awning-private-selection.js";

const digest = "a".repeat(64);
const generation = 7;
const valid = () => ({
  schema_version: 1,
  purpose: "read_only_awning_operator_selection",
  generation,
  selected_homey_id_sha256: digest,
  selections: {
    awning_1: "device_111111111111",
    awning_2: "device_222222222222",
    awning_3: "device_333333333333",
  },
});

test("Patch042 private selection accepts exactly three unique canonical device aliases", () => {
  const parsed = validatePatch042PrivateSelection(valid(), digest, generation);
  assert.equal(parsed.selections.awning_1, "device_111111111111");
  assert.equal(parsed.selections.awning_3, "device_333333333333");
});

test("Patch042 private selection refuses digest, generation, duplicate, alias and unknown-field drift", () => {
  assert.throws(() => validatePatch042PrivateSelection(valid(), "b".repeat(64), generation), /selected-Homey mismatch/);
  assert.throws(() => validatePatch042PrivateSelection(valid(), digest, generation + 1), /generation mismatch/);

  const duplicate = valid();
  duplicate.selections.awning_3 = duplicate.selections.awning_1;
  assert.throws(() => validatePatch042PrivateSelection(duplicate, digest, generation), /three unique devices/);

  const invalidAlias = valid();
  invalidAlias.selections.awning_2 = "raw-homey-device-id";
  assert.throws(() => validatePatch042PrivateSelection(invalidAlias, digest, generation), /canonical device alias/);

  assert.throws(
    () => validatePatch042PrivateSelection({ ...valid(), extra: true }, digest, generation),
    /unknown or missing fields/,
  );
});

test("Patch042 private selection loader requires outside-repository 0600 regular file", async () => {
  const repo = await mkdtemp(join(tmpdir(), "patch042-selection-repo-"));
  const outside = await mkdtemp(join(tmpdir(), "patch042-selection-private-"));
  const path = join(outside, "awning_selection.json");
  await writeFile(path, JSON.stringify(valid()), { mode: 0o600 });
  await chmod(path, 0o600);
  const loaded = await loadPatch042PrivateSelection(path, repo, digest, generation);
  assert.equal(loaded.selections.awning_2, "device_222222222222");

  await chmod(path, 0o644);
  await assert.rejects(
    loadPatch042PrivateSelection(path, repo, digest, generation),
    /permissions/,
  );

  const inside = join(repo, "selection.json");
  await writeFile(inside, JSON.stringify(valid()), { mode: 0o600 });
  await chmod(inside, 0o600);
  await assert.rejects(
    loadPatch042PrivateSelection(inside, repo, digest, generation),
    /outside the repository/,
  );
});
