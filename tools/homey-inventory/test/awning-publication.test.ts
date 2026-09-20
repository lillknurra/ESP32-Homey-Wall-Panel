import assert from "node:assert/strict";
import { createHash } from "node:crypto";
import { mkdtemp, readFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";
import test from "node:test";
import { buildAwningEvidence } from "../src/awning-evidence-builder.js";
import { publishAwningEvidence } from "../src/awning-evidence-publication.js";
import { collectPatch039ReadSurface } from "../src/awning-readonly-client.js";
import { wrapVerifiedPatch039HomeyApi } from "../src/awning-homey-api-adapter.js";
import type { PrivateAwningMapping } from "../src/awning-private-mapping.js";

const rawHomeyId = "synthetic-homey-identity-for-publication";
const digest = createHash("sha256").update(rawHomeyId).digest("hex");
const mapping: PrivateAwningMapping = {
  schema_version: 1,
  purpose: "read_only_awning_evidence",
  generation: 3,
  selected_homey_id_sha256: digest,
  mappings: {
    awning_1: "synthetic-publish-device-a",
    awning_2: "synthetic-publish-device-b",
    awning_3: "synthetic-publish-device-c",
  },
};

function device(id: string) {
  return {
    id,
    available: true,
    class: "blinds",
    driverId: "synthetic-publish-driver",
    capabilities: ["command"],
    capabilitiesObj: {
      command: {
        type: "enum",
        setable: true,
        values: [
          { id: "raise", title: "Open" },
          { id: "halt", title: "Stop" },
          { id: "lower", title: "Close" },
        ],
        value: "halt",
      },
    },
  };
}

function fakeApi(overrides: { failFlows?: boolean } = {}) {
  return {
    id: rawHomeyId,
    devices: {
      async getDevices() {
        return [device(mapping.mappings.awning_1), device(mapping.mappings.awning_2), device(mapping.mappings.awning_3)];
      },
      async getCapabilityValue() { return null; },
    },
    flow: {
      async getFlows() {
        if (overrides.failFlows) throw new Error("synthetic read failure");
        return [];
      },
      async getFlowCardActions() { return []; },
      async getAdvancedFlows() { return []; },
    },
  };
}

async function buildCleanEvidence() {
  const client = wrapVerifiedPatch039HomeyApi(fakeApi(), digest);
  const raw = await collectPatch039ReadSurface(client);
  return buildAwningEvidence({
    raw,
    mapping,
    expectedHomeyDigest: digest,
    registry: {},
    client,
    generation: 11,
  });
}

test("complete synthetic collection publishes atomically outside repo with only sanitized evidence", async () => {
  const repo = await mkdtemp(join(tmpdir(), "patch039-pub-repo-"));
  const outside = await mkdtemp(join(tmpdir(), "patch039-pub-private-"));
  const output = join(outside, "accepted");
  const built = await buildCleanEvidence();
  assert.equal(built.evidence.actions.awning_1.OPEN.classification, "CANDIDATE_DEVICE_CAPABILITY");
  assert.equal(built.evidence.actions.awning_1.STOP.classification, "CANDIDATE_DEVICE_CAPABILITY");
  assert.equal(built.evidence.actions.awning_1.CLOSE.classification, "CANDIDATE_DEVICE_CAPABILITY");
  await publishAwningEvidence(output, repo, built.evidence, built.rawIds);
  const serialized = await readFile(join(output, "awning_evidence.json"), "utf8");
  const parsed = JSON.parse(serialized) as { session: { publication_attempted: boolean } };
  assert.equal(parsed.session.publication_attempted, true);
  assert.equal(serialized.includes(rawHomeyId), false);
  for (const rawId of Object.values(mapping.mappings)) assert.equal(serialized.includes(rawId), false);
  assert.equal(serialized.includes("raise"), false);
  assert.equal(serialized.includes("halt"), false);
  assert.equal(serialized.includes("lower"), false);
});

test("raw-ID leak or invalid candidate never replaces previously accepted evidence", async () => {
  const repo = await mkdtemp(join(tmpdir(), "patch039-pub-repo-"));
  const outside = await mkdtemp(join(tmpdir(), "patch039-pub-private-"));
  const output = join(outside, "accepted");
  const built = await buildCleanEvidence();
  await publishAwningEvidence(output, repo, built.evidence, built.rawIds);
  const before = await readFile(join(output, "awning_evidence.json"), "utf8");

  const leaking = structuredClone(built.evidence);
  leaking.actions.awning_1.OPEN.classification = "INSUFFICIENT_EVIDENCE";
  leaking.actions.awning_1.OPEN.blocking_reasons = ["RAW_HOMEY_ID"];
  await assert.rejects(
    publishAwningEvidence(output, repo, leaking, [...built.rawIds, "RAW_HOMEY_ID"]),
    /Raw Homey identifier leaked/,
  );
  assert.equal(await readFile(join(output, "awning_evidence.json"), "utf8"), before);

  const invalid = structuredClone(built.evidence) as unknown as Record<string, unknown>;
  invalid.purpose = "forbidden-purpose";
  await assert.rejects(
    publishAwningEvidence(output, repo, invalid as unknown as typeof built.evidence, built.rawIds),
    /schema or purpose mismatch/,
  );
  assert.equal(await readFile(join(output, "awning_evidence.json"), "utf8"), before);
});

test("extra nested private field is rejected and previous accepted evidence is preserved", async () => {
  const repo = await mkdtemp(join(tmpdir(), "patch039-pub-repo-"));
  const outside = await mkdtemp(join(tmpdir(), "patch039-pub-private-"));
  const output = join(outside, "accepted");
  const built = await buildCleanEvidence();
  await publishAwningEvidence(output, repo, built.evidence, built.rawIds);
  const before = await readFile(join(output, "awning_evidence.json"), "utf8");

  const nested = structuredClone(built.evidence) as unknown as {
    devices: Array<Record<string, unknown>>;
  };
  nested.devices[0]!.unexpected_private = "installation-specific-private-value";
  await assert.rejects(
    publishAwningEvidence(output, repo, nested as unknown as typeof built.evidence, built.rawIds),
    /device keys are invalid/,
  );
  assert.equal(await readFile(join(output, "awning_evidence.json"), "utf8"), before);
});

test("invalid nested aliases and ledger failure classes are rejected before replacement", async () => {
  const repo = await mkdtemp(join(tmpdir(), "patch039-pub-repo-"));
  const outside = await mkdtemp(join(tmpdir(), "patch039-pub-private-"));
  const output = join(outside, "accepted");
  const built = await buildCleanEvidence();
  await publishAwningEvidence(output, repo, built.evidence, built.rawIds);
  const before = await readFile(join(output, "awning_evidence.json"), "utf8");

  const aliasLeak = structuredClone(built.evidence);
  aliasLeak.devices[0]!.device_alias = "device_notcanonical";
  await assert.rejects(publishAwningEvidence(output, repo, aliasLeak, built.rawIds), /device alias is invalid/);
  assert.equal(await readFile(join(output, "awning_evidence.json"), "utf8"), before);

  const ledgerLeak = structuredClone(built.evidence) as unknown as {
    call_ledger: Array<Record<string, unknown>>;
  };
  ledgerLeak.call_ledger[0]!.result = "failure";
  ledgerLeak.call_ledger[0]!.aggregate_count = null;
  ledgerLeak.call_ledger[0]!.failure_class = "PRIVATE_FAILURE";
  await assert.rejects(
    publishAwningEvidence(output, repo, ledgerLeak as unknown as typeof built.evidence, built.rawIds),
    /failure_class is outside the bounded failure domain/,
  );
  assert.equal(await readFile(join(output, "awning_evidence.json"), "utf8"), before);
});

test("partial base collection cannot be built or published over accepted evidence", async () => {
  const repo = await mkdtemp(join(tmpdir(), "patch039-pub-repo-"));
  const outside = await mkdtemp(join(tmpdir(), "patch039-pub-private-"));
  const output = join(outside, "accepted");
  const clean = await buildCleanEvidence();
  await publishAwningEvidence(output, repo, clean.evidence, clean.rawIds);
  const before = await readFile(join(output, "awning_evidence.json"), "utf8");

  const failingClient = wrapVerifiedPatch039HomeyApi(fakeApi({ failFlows: true }), digest);
  await assert.rejects(collectPatch039ReadSurface(failingClient), /allowlisted Homey read failed/);
  assert.equal(await readFile(join(output, "awning_evidence.json"), "utf8"), before);
});

test("Patch039 evidence publication refuses repository-local output before creating artifacts", async () => {
  const repo = await mkdtemp(join(tmpdir(), "patch039-pub-repo-"));
  const output = join(repo, "forbidden-output");
  const built = await buildCleanEvidence();
  await assert.rejects(publishAwningEvidence(output, repo, built.evidence, built.rawIds), /outside the repository/);
});

test("short raw capability identifiers are checked as values without substring false positives", async () => {
  const repo = await mkdtemp(join(tmpdir(), "patch039-pub-repo-"));
  const outside = await mkdtemp(join(tmpdir(), "patch039-pub-private-"));
  const output = join(outside, "accepted");
  const built = await buildCleanEvidence();
  await assert.doesNotReject(publishAwningEvidence(output, repo, built.evidence, [...built.rawIds, "up"]));

  const leak = structuredClone(built.evidence);
  leak.actions.awning_1.OPEN.classification = "INSUFFICIENT_EVIDENCE";
  leak.actions.awning_1.OPEN.blocking_reasons = ["UP"];
  await assert.rejects(publishAwningEvidence(output, repo, leak, [...built.rawIds, "UP"]), /Raw Homey identifier leaked/);
});

test("secret-pattern leak is refused before accepted evidence replacement", async () => {
  const repo = await mkdtemp(join(tmpdir(), "patch039-pub-repo-"));
  const outside = await mkdtemp(join(tmpdir(), "patch039-pub-private-"));
  const output = join(outside, "accepted");
  const built = await buildCleanEvidence();
  await publishAwningEvidence(output, repo, built.evidence, built.rawIds);
  const before = await readFile(join(output, "awning_evidence.json"), "utf8");
  const leak = structuredClone(built.evidence);
  leak.actions.awning_1.OPEN.blocking_reasons = [["Bearer", "synthetic.secret"].join(" ")];
  await assert.rejects(publishAwningEvidence(output, repo, leak, built.rawIds), /invalid string|Sensitive value pattern/);
  assert.equal(await readFile(join(output, "awning_evidence.json"), "utf8"), before);
});
