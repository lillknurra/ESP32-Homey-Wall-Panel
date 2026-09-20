import { lstat, mkdir, realpath, rename, rm, writeFile } from "node:fs/promises";
import { basename, dirname, isAbsolute, join, relative, resolve } from "node:path";
import {
  AWNING_ACTIONS,
  AWNING_CANDIDATE_CLASSIFICATIONS,
  AWNING_ROLES,
  type AwningEvidenceDocument,
} from "./awning-model.js";
import { PATCH039_ALLOWED_READ_OPERATIONS } from "./awning-call-ledger.js";
import { CandidateError } from "./errors.js";
import { redact } from "./redaction.js";

const SAFE_DEVICE_CLASSES = new Set([
  "blinds", "curtain", "light", "other", "sensor", "socket", "sunshade", "windowcoverings",
]);
const CAPABILITY_VALUE_TYPES = new Set(["null", "boolean", "number", "string", "unknown"]);
const CAPABILITY_METADATA_KEYS = new Set(["type", "getable", "setable", "min", "max", "step", "values", "units", "options"]);
const ADVANCED_CARD_TYPES = new Set(["action", "condition", "trigger", "unknown"]);
const FAILURE_CLASSES = new Set([
  "DISCOVERY", "REACHABILITY", "AUTHENTICATION", "AUTHORIZATION", "TLS",
  "API_INCOMPATIBILITY", "MALFORMED_RESPONSE", "SCHEMA_MISMATCH", "CONFIGURATION",
]);
const SAFE_FIELD_NAME = /^[A-Za-z0-9_.:-]{1,64}$/;
const SAFE_BLOCKER = /^[A-Z0-9_]{1,96}$/;

function recordOf(value: unknown): Record<string, unknown> | null {
  return value && typeof value === "object" && !Array.isArray(value) ? value as Record<string, unknown> : null;
}

function exactKeys(record: Record<string, unknown>, expected: readonly string[], label: string): void {
  const actual = Object.keys(record).sort();
  const wanted = [...expected].sort();
  if (JSON.stringify(actual) !== JSON.stringify(wanted)) {
    throw new CandidateError("SCHEMA_MISMATCH", `Patch039 evidence ${label} keys are invalid`);
  }
}

function isInside(parent: string, candidate: string): boolean {
  const rel = relative(resolve(parent), resolve(candidate));
  return rel === "" || (!rel.startsWith("..") && !isAbsolute(rel));
}

function assertCanonicalAlias(value: unknown, kinds: readonly string[], label: string): asserts value is string {
  if (typeof value !== "string" || !kinds.some((kind) => new RegExp(`^${kind}_[0-9a-f]{12}$`).test(value))) {
    throw new CandidateError("SCHEMA_MISMATCH", `Patch039 evidence ${label} alias is invalid`);
  }
}

function assertNullableCanonicalAlias(value: unknown, kinds: readonly string[], label: string): void {
  if (value === null) return;
  assertCanonicalAlias(value, kinds, label);
}

function assertBooleanOrNull(value: unknown, label: string): void {
  if (value !== null && typeof value !== "boolean") {
    throw new CandidateError("SCHEMA_MISMATCH", `Patch039 evidence ${label} must be boolean or null`);
  }
}

function assertNonNegativeInteger(value: unknown, label: string): asserts value is number {
  if (!Number.isInteger(value) || Number(value) < 0) {
    throw new CandidateError("SCHEMA_MISMATCH", `Patch039 evidence ${label} must be a non-negative integer`);
  }
}

function assertSortedUniqueStrings(
  value: unknown,
  label: string,
  options: { maxItems?: number; pattern?: RegExp; allowed?: ReadonlySet<string> } = {},
): asserts value is string[] {
  if (!Array.isArray(value) || value.some((item) => typeof item !== "string")) {
    throw new CandidateError("SCHEMA_MISMATCH", `Patch039 evidence ${label} must be a string array`);
  }
  if (options.maxItems !== undefined && value.length > options.maxItems) {
    throw new CandidateError("SCHEMA_MISMATCH", `Patch039 evidence ${label} is too large`);
  }
  if (new Set(value).size !== value.length || JSON.stringify([...value].sort()) !== JSON.stringify(value)) {
    throw new CandidateError("SCHEMA_MISMATCH", `Patch039 evidence ${label} must be unique and sorted`);
  }
  for (const item of value) {
    if (options.pattern && !options.pattern.test(item)) {
      throw new CandidateError("SCHEMA_MISMATCH", `Patch039 evidence ${label} contains an invalid string`);
    }
    if (options.allowed && !options.allowed.has(item)) {
      throw new CandidateError("SCHEMA_MISMATCH", `Patch039 evidence ${label} contains an unsupported value`);
    }
  }
}

function assertTargetRoles(value: unknown, label: string): void {
  assertSortedUniqueStrings(value, label, { maxItems: 3, allowed: new Set(AWNING_ROLES) });
}

function assertCapabilityEvidence(value: unknown): void {
  const capability = recordOf(value);
  if (!capability) throw new CandidateError("SCHEMA_MISMATCH", "Patch039 capability evidence must be an object");
  exactKeys(capability, ["capability_alias", "value_type", "value", "last_updated_present", "observed_metadata_keys"], "capability");
  assertCanonicalAlias(capability.capability_alias, ["capability"], "capability");
  if (typeof capability.value_type !== "string" || !CAPABILITY_VALUE_TYPES.has(capability.value_type)) {
    throw new CandidateError("SCHEMA_MISMATCH", "Patch039 capability value_type is invalid");
  }
  if (capability.value_type === "boolean") {
    if (typeof capability.value !== "boolean") throw new CandidateError("SCHEMA_MISMATCH", "Patch039 boolean capability value is invalid");
  } else if (capability.value_type === "number") {
    if (typeof capability.value !== "number" || !Number.isFinite(capability.value)) {
      throw new CandidateError("SCHEMA_MISMATCH", "Patch039 number capability value is invalid");
    }
  } else if (capability.value !== null) {
    throw new CandidateError("SCHEMA_MISMATCH", "Patch039 non-primitive-safe capability value must be null");
  }
  if (typeof capability.last_updated_present !== "boolean") {
    throw new CandidateError("SCHEMA_MISMATCH", "Patch039 last_updated_present must be boolean");
  }
  assertSortedUniqueStrings(capability.observed_metadata_keys, "observed_metadata_keys", {
    maxItems: CAPABILITY_METADATA_KEYS.size,
    allowed: CAPABILITY_METADATA_KEYS,
  });
}

function assertDeviceEvidence(value: unknown): string {
  const device = recordOf(value);
  if (!device) throw new CandidateError("SCHEMA_MISMATCH", "Patch039 device evidence must be an object");
  exactKeys(device, ["role", "device_alias", "available", "class", "driver_alias", "capabilities"], "device");
  if (typeof device.role !== "string" || !AWNING_ROLES.includes(device.role as (typeof AWNING_ROLES)[number])) {
    throw new CandidateError("SCHEMA_MISMATCH", "Patch039 evidence contains an invalid awning role");
  }
  assertCanonicalAlias(device.device_alias, ["device"], "device");
  assertBooleanOrNull(device.available, "device.available");
  if (device.class !== null && (typeof device.class !== "string" || !SAFE_DEVICE_CLASSES.has(device.class))) {
    throw new CandidateError("SCHEMA_MISMATCH", "Patch039 device class is invalid");
  }
  assertNullableCanonicalAlias(device.driver_alias, ["driver"], "driver");
  if (!Array.isArray(device.capabilities)) throw new CandidateError("SCHEMA_MISMATCH", "Patch039 device capabilities must be an array");
  const aliases = new Set<string>();
  for (const capability of device.capabilities) {
    assertCapabilityEvidence(capability);
    const alias = (capability as Record<string, unknown>).capability_alias as string;
    if (aliases.has(alias)) throw new CandidateError("SCHEMA_MISMATCH", "Patch039 capability aliases must be unique per device");
    aliases.add(alias);
  }
  return device.role;
}

function assertFlowActionEvidence(value: unknown): void {
  const action = recordOf(value);
  if (!action) throw new CandidateError("SCHEMA_MISMATCH", "Patch039 Flow action evidence must be an object");
  exactKeys(action, ["card_alias", "owner_alias", "target_roles", "referenced_device_count", "unknown_field_names"], "Flow action");
  assertCanonicalAlias(action.card_alias, ["flow_card"], "Flow card");
  assertNullableCanonicalAlias(action.owner_alias, ["owner"], "Flow owner");
  assertTargetRoles(action.target_roles, "Flow target_roles");
  assertNonNegativeInteger(action.referenced_device_count, "Flow referenced_device_count");
  if (Number(action.referenced_device_count) < (action.target_roles as string[]).length) {
    throw new CandidateError("SCHEMA_MISMATCH", "Patch039 Flow referenced_device_count is inconsistent");
  }
  assertSortedUniqueStrings(action.unknown_field_names, "Flow unknown_field_names", { maxItems: 32, pattern: SAFE_FIELD_NAME });
}

function assertFlowEvidence(value: unknown): void {
  const flow = recordOf(value);
  if (!flow) throw new CandidateError("SCHEMA_MISMATCH", "Patch039 Flow evidence must be an object");
  exactKeys(flow, ["flow_alias", "enabled", "triggerable", "action_count", "actions"], "Flow");
  assertCanonicalAlias(flow.flow_alias, ["flow"], "Flow");
  assertBooleanOrNull(flow.enabled, "Flow.enabled");
  assertBooleanOrNull(flow.triggerable, "Flow.triggerable");
  assertNonNegativeInteger(flow.action_count, "Flow action_count");
  if (!Array.isArray(flow.actions) || flow.actions.length !== flow.action_count) {
    throw new CandidateError("SCHEMA_MISMATCH", "Patch039 Flow action_count does not match sanitized actions");
  }
  for (const action of flow.actions) assertFlowActionEvidence(action);
}

function assertAdvancedFlowCardEvidence(value: unknown): void {
  const card = recordOf(value);
  if (!card) throw new CandidateError("SCHEMA_MISMATCH", "Patch039 Advanced Flow card evidence must be an object");
  exactKeys(card, ["card_alias", "owner_alias", "type", "target_roles", "referenced_device_count", "unknown_field_names"], "Advanced Flow card");
  assertCanonicalAlias(card.card_alias, ["advanced_flow_card"], "Advanced Flow card");
  assertNullableCanonicalAlias(card.owner_alias, ["owner"], "Advanced Flow owner");
  if (typeof card.type !== "string" || !ADVANCED_CARD_TYPES.has(card.type)) {
    throw new CandidateError("SCHEMA_MISMATCH", "Patch039 Advanced Flow card type is invalid");
  }
  assertTargetRoles(card.target_roles, "Advanced Flow target_roles");
  assertNonNegativeInteger(card.referenced_device_count, "Advanced Flow referenced_device_count");
  if (Number(card.referenced_device_count) < (card.target_roles as string[]).length) {
    throw new CandidateError("SCHEMA_MISMATCH", "Patch039 Advanced Flow referenced_device_count is inconsistent");
  }
  assertSortedUniqueStrings(card.unknown_field_names, "Advanced Flow unknown_field_names", { maxItems: 32, pattern: SAFE_FIELD_NAME });
}

function assertAdvancedFlowEvidence(value: unknown): void {
  const advanced = recordOf(value);
  if (!advanced) throw new CandidateError("SCHEMA_MISMATCH", "Patch039 Advanced Flow evidence must be an object");
  exactKeys(advanced, ["advanced_flow_alias", "enabled", "triggerable", "card_count", "cards"], "Advanced Flow");
  assertCanonicalAlias(advanced.advanced_flow_alias, ["advanced_flow"], "Advanced Flow");
  assertBooleanOrNull(advanced.enabled, "Advanced Flow.enabled");
  assertBooleanOrNull(advanced.triggerable, "Advanced Flow.triggerable");
  assertNonNegativeInteger(advanced.card_count, "Advanced Flow card_count");
  if (!Array.isArray(advanced.cards) || advanced.cards.length !== advanced.card_count) {
    throw new CandidateError("SCHEMA_MISMATCH", "Patch039 Advanced Flow card_count does not match sanitized cards");
  }
  for (const card of advanced.cards) assertAdvancedFlowCardEvidence(card);
}

function assertActionEvidence(value: unknown, role: string, action: string): void {
  const item = recordOf(value);
  if (!item) throw new CandidateError("SCHEMA_MISMATCH", "Patch039 evidence action entry must be an object");
  exactKeys(item, ["classification", "candidate_source_aliases", "blocking_reasons"], `${role}.${action}`);
  if (typeof item.classification !== "string"
      || !AWNING_CANDIDATE_CLASSIFICATIONS.includes(item.classification as (typeof AWNING_CANDIDATE_CLASSIFICATIONS)[number])) {
    throw new CandidateError("SCHEMA_MISMATCH", "Patch039 evidence classification is outside the locked allowlist");
  }
  assertSortedUniqueStrings(item.candidate_source_aliases, "candidate_source_aliases", { maxItems: 32 });
  for (const alias of item.candidate_source_aliases as string[]) {
    assertCanonicalAlias(alias, ["capability", "flow", "advanced_flow"], "candidate source");
  }
  assertSortedUniqueStrings(item.blocking_reasons, "blocking_reasons", { maxItems: 32, pattern: SAFE_BLOCKER });
  const aliases = item.candidate_source_aliases as string[];
  const blockers = item.blocking_reasons as string[];
  if (item.classification === "NO_CANDIDATE" && (aliases.length !== 0 || blockers.length !== 0)) {
    throw new CandidateError("SCHEMA_MISMATCH", "Patch039 NO_CANDIDATE must not carry candidate aliases or blockers");
  }
  if ((item.classification === "CANDIDATE_DEVICE_CAPABILITY"
      || item.classification === "CANDIDATE_FLOW"
      || item.classification === "CANDIDATE_ADVANCED_FLOW")
      && (aliases.length !== 1 || blockers.length !== 0)) {
    throw new CandidateError("SCHEMA_MISMATCH", "Patch039 positive candidate must be singular and unblocked");
  }
  if (item.classification === "CANDIDATE_DEVICE_CAPABILITY") assertCanonicalAlias(aliases[0], ["capability"], "device-capability candidate source");
  if (item.classification === "CANDIDATE_FLOW") assertCanonicalAlias(aliases[0], ["flow"], "Flow candidate source");
  if (item.classification === "CANDIDATE_ADVANCED_FLOW") assertCanonicalAlias(aliases[0], ["advanced_flow"], "Advanced Flow candidate source");
}

export function assertAwningEvidenceSchema(value: unknown): asserts value is AwningEvidenceDocument {
  const document = recordOf(value);
  if (!document) throw new CandidateError("SCHEMA_MISMATCH", "Patch039 evidence must be an object");
  exactKeys(document, [
    "schema_version", "purpose", "generation", "selected_homey_verified", "collection_complete",
    "devices", "flows", "advanced_flows", "actions", "call_ledger", "session", "privacy_report",
  ], "top-level");
  if (document.schema_version !== "1.0.0" || document.purpose !== "read_only_awning_evidence") {
    throw new CandidateError("SCHEMA_MISMATCH", "Patch039 evidence schema or purpose mismatch");
  }
  if (!Number.isInteger(document.generation) || Number(document.generation) <= 0) {
    throw new CandidateError("SCHEMA_MISMATCH", "Patch039 evidence generation must be positive");
  }
  if (document.selected_homey_verified !== true || typeof document.collection_complete !== "boolean") {
    throw new CandidateError("SCHEMA_MISMATCH", "Patch039 evidence authority flags are invalid");
  }

  if (!Array.isArray(document.devices) || document.devices.length !== AWNING_ROLES.length) {
    throw new CandidateError("SCHEMA_MISMATCH", "Patch039 evidence must contain exactly three mapped awnings");
  }
  const roles = new Set<string>();
  for (const item of document.devices) roles.add(assertDeviceEvidence(item));
  if (roles.size !== AWNING_ROLES.length) {
    throw new CandidateError("SCHEMA_MISMATCH", "Patch039 evidence mapped awning roles must be unique");
  }

  if (!Array.isArray(document.flows)) throw new CandidateError("SCHEMA_MISMATCH", "Patch039 flows must be an array");
  for (const flow of document.flows) assertFlowEvidence(flow);
  if (!Array.isArray(document.advanced_flows)) throw new CandidateError("SCHEMA_MISMATCH", "Patch039 advanced_flows must be an array");
  for (const advanced of document.advanced_flows) assertAdvancedFlowEvidence(advanced);

  const actions = recordOf(document.actions);
  if (!actions) throw new CandidateError("SCHEMA_MISMATCH", "Patch039 evidence actions must be an object");
  exactKeys(actions, AWNING_ROLES, "actions");
  for (const role of AWNING_ROLES) {
    const roleActions = recordOf(actions[role]);
    if (!roleActions) throw new CandidateError("SCHEMA_MISMATCH", "Patch039 evidence role actions must be objects");
    exactKeys(roleActions, AWNING_ACTIONS, `${role} actions`);
    for (const action of AWNING_ACTIONS) assertActionEvidence(roleActions[action], role, action);
  }

  if (!Array.isArray(document.call_ledger)) {
    throw new CandidateError("SCHEMA_MISMATCH", "Patch039 evidence call ledger must be an array");
  }
  let expectedSeq = 1;
  for (const rawEntry of document.call_ledger) {
    const entry = recordOf(rawEntry);
    if (!entry) throw new CandidateError("SCHEMA_MISMATCH", "Patch039 evidence ledger entry must be an object");
    exactKeys(entry, ["seq", "operation", "result", "aggregate_count", "failure_class"], "ledger entry");
    if (entry.seq !== expectedSeq++) throw new CandidateError("SCHEMA_MISMATCH", "Patch039 ledger sequence is invalid");
    if (typeof entry.operation !== "string"
        || !PATCH039_ALLOWED_READ_OPERATIONS.includes(entry.operation as (typeof PATCH039_ALLOWED_READ_OPERATIONS)[number])) {
      throw new CandidateError("SCHEMA_MISMATCH", "Patch039 ledger operation is outside the read allowlist");
    }
    if (entry.result !== "success" && entry.result !== "failure") {
      throw new CandidateError("SCHEMA_MISMATCH", "Patch039 ledger result is invalid");
    }
    if (entry.aggregate_count !== null && (!Number.isInteger(entry.aggregate_count) || Number(entry.aggregate_count) < 0)) {
      throw new CandidateError("SCHEMA_MISMATCH", "Patch039 ledger aggregate_count is invalid");
    }
    if (entry.failure_class !== null && (typeof entry.failure_class !== "string" || !FAILURE_CLASSES.has(entry.failure_class))) {
      throw new CandidateError("SCHEMA_MISMATCH", "Patch039 ledger failure_class is outside the bounded failure domain");
    }
    if (entry.result === "success" && entry.failure_class !== null) {
      throw new CandidateError("SCHEMA_MISMATCH", "Patch039 successful ledger entry must not carry failure_class");
    }
    if (entry.result === "failure" && (entry.failure_class === null || entry.aggregate_count !== null)) {
      throw new CandidateError("SCHEMA_MISMATCH", "Patch039 failed ledger entry is malformed");
    }
    if (entry.operation === "capability_value.read" && entry.aggregate_count !== null) {
      throw new CandidateError("SCHEMA_MISMATCH", "Patch039 capability_value.read must not expose aggregate data");
    }
  }

  const session = recordOf(document.session);
  if (!session) throw new CandidateError("SCHEMA_MISMATCH", "Patch039 evidence session must be an object");
  exactKeys(session, [
    "authentication_attempted", "authentication_succeeded", "selected_homey_verified",
    "collection_attempted", "publication_attempted",
  ], "session");
  for (const key of Object.keys(session)) {
    if (typeof session[key] !== "boolean") throw new CandidateError("SCHEMA_MISMATCH", "Patch039 session fields must be booleans");
  }
  if (session.authentication_attempted !== true || session.authentication_succeeded !== true
      || session.selected_homey_verified !== true || session.collection_attempted !== true
      || session.publication_attempted !== true) {
    throw new CandidateError("SCHEMA_MISMATCH", "Patch039 evidence was not fully authorized for read-only publication");
  }

  const privacy = recordOf(document.privacy_report);
  if (!privacy) throw new CandidateError("SCHEMA_MISMATCH", "Patch039 privacy report must be an object");
  exactKeys(privacy, ["raw_ids_present", "secrets_present", "unknown_open_schema_values_passed_through"], "privacy report");
  if (privacy.raw_ids_present !== false || privacy.secrets_present !== false
      || privacy.unknown_open_schema_values_passed_through !== false) {
    throw new CandidateError("SCHEMA_MISMATCH", "Patch039 privacy report must remain fail-closed");
  }
}

function assertNoRawIdentifiers(value: unknown, rawIds: readonly string[]): void {
  const unique = [...new Set(rawIds.filter((item) => item.length > 0))];
  const visit = (item: unknown): void => {
    if (typeof item === "string") {
      for (const rawId of unique) {
        if (item === rawId || (rawId.length >= 8 && item.includes(rawId))) {
          throw new CandidateError("SCHEMA_MISMATCH", "Raw Homey identifier leaked into Patch039 evidence");
        }
      }
      return;
    }
    if (Array.isArray(item)) {
      for (const nested of item) visit(nested);
      return;
    }
    const record = recordOf(item);
    if (record) {
      for (const [key, nested] of Object.entries(record)) {
        visit(key);
        visit(nested);
      }
    }
  };
  visit(value);
}

async function assertPrivateOutputPath(outputDir: string, repositoryRoot: string): Promise<string> {
  if (!isAbsolute(outputDir)) throw new CandidateError("CONFIGURATION", "Patch039 output path must be absolute");
  const canonicalRepository = await realpath(repositoryRoot);
  const canonicalParent = await realpath(dirname(resolve(outputDir)));
  const candidate = join(canonicalParent, basename(outputDir));
  if (isInside(canonicalRepository, candidate)) {
    throw new CandidateError("CONFIGURATION", "Patch039 output must remain outside the repository");
  }
  try {
    const existing = await lstat(candidate);
    if (!existing.isDirectory() || existing.isSymbolicLink()) {
      throw new CandidateError("CONFIGURATION", "Patch039 output must be a real directory");
    }
    if ((existing.mode & 0o077) !== 0) {
      throw new CandidateError("CONFIGURATION", "Patch039 output directory permissions are too broad");
    }
  } catch (error) {
    const code = (error as NodeJS.ErrnoException).code;
    if (code !== "ENOENT") throw error;
  }
  return candidate;
}

export async function publishAwningEvidence(
  outputDir: string,
  repositoryRoot: string,
  evidence: AwningEvidenceDocument,
  rawIds: readonly string[],
): Promise<void> {
  const privateOutput = await assertPrivateOutputPath(outputDir, repositoryRoot);
  const candidateEvidence: AwningEvidenceDocument = {
    ...evidence,
    session: { ...evidence.session, publication_attempted: true },
  };
  assertAwningEvidenceSchema(candidateEvidence);
  assertNoRawIdentifiers(candidateEvidence, rawIds);
  const serialized = `${JSON.stringify(candidateEvidence, null, 2)}\n`;
  if (redact(serialized) !== serialized) {
    throw new CandidateError("SCHEMA_MISMATCH", "Sensitive value pattern detected in Patch039 evidence");
  }

  await mkdir(privateOutput, { recursive: true, mode: 0o700 });
  const nonce = `${process.pid}-${Date.now()}`;
  const tmp = join(privateOutput, `.awning_evidence.${nonce}.json.tmp`);
  const finalPath = join(privateOutput, "awning_evidence.json");
  try {
    await writeFile(tmp, serialized, { mode: 0o600, flag: "wx" });
    await rename(tmp, finalPath);
  } finally {
    await rm(tmp, { force: true });
  }
}
