import { patch039AliasFor, type AliasRegistry } from "./aliases.js";
import {
  AWNING_ACTIONS,
  AWNING_ROLES,
  type AwningAction,
  type AwningRole,
  type CandidateObservation,
  type RawAwningCollection,
  type SanitizedAdvancedFlowEvidence,
  type SanitizedCapabilityEvidence,
  type SanitizedDeviceEvidence,
  type SanitizedFlowEvidence,
} from "./awning-model.js";
import type { PrivateAwningMapping } from "./awning-private-mapping.js";

const SAFE_DEVICE_CLASSES = new Set([
  "blinds", "curtain", "light", "other", "sensor", "socket", "sunshade", "windowcoverings",
]);
const CAPABILITY_ALLOWED_FIELDS = new Set([
  "id", "title", "type", "getable", "setable", "min", "max", "step", "values", "units", "options", "value", "lastUpdated",
]);
const CAPABILITY_METADATA_KEYS = new Set([
  "type", "getable", "setable", "min", "max", "step", "values", "units", "options",
]);
const FLOW_CARD_DESCRIPTOR_ALLOWED_FIELDS = new Set([
  "id", "ownerId", "ownerUri", "ownerName", "title", "titleFormatted", "hint", "args", "droptoken",
  "deprecated", "highlight", "duration", "durationMin", "durationMax", "tokens", "advanced",
]);

function recordOf(value: unknown): Record<string, unknown> | null {
  return value && typeof value === "object" && !Array.isArray(value) ? value as Record<string, unknown> : null;
}

function objectId(value: Record<string, unknown>): string | null {
  const id = value.id ?? value._id;
  return typeof id === "string" && id.length > 0 ? id : null;
}

function booleanOrNull(value: unknown): boolean | null {
  return typeof value === "boolean" ? value : null;
}

function valueEvidence(value: unknown): Pick<SanitizedCapabilityEvidence, "value_type" | "value"> {
  if (value === null) return { value_type: "null", value: null };
  if (typeof value === "boolean") return { value_type: "boolean", value };
  if (typeof value === "number" && Number.isFinite(value)) return { value_type: "number", value };
  if (typeof value === "string") return { value_type: "string", value: null };
  return { value_type: "unknown", value: null };
}

function unknownFields(value: Record<string, unknown>, allowed: ReadonlySet<string>): string[] {
  return Object.keys(value).filter((key) => !allowed.has(key)).sort();
}

function findDeviceReferences(value: unknown, knownDeviceIds: ReadonlySet<string>, out = new Set<string>()): Set<string> {
  if (typeof value === "string") {
    if (knownDeviceIds.has(value)) out.add(value);
    return out;
  }
  if (Array.isArray(value)) {
    for (const item of value) findDeviceReferences(item, knownDeviceIds, out);
    return out;
  }
  const record = recordOf(value);
  if (record) for (const nested of Object.values(record)) findDeviceReferences(nested, knownDeviceIds, out);
  return out;
}

interface SavedArgsInspection {
  references: Set<string>;
  verifiedSingleDeviceTarget: boolean;
  blockers: string[];
}

function inspectSavedArgs(value: unknown, knownDeviceIds: ReadonlySet<string>, blocker: string): SavedArgsInspection {
  const references = findDeviceReferences(value, knownDeviceIds);
  if (value === undefined) {
    return { references, verifiedSingleDeviceTarget: false, blockers: [] };
  }
  const record = recordOf(value);
  if (!record) {
    return { references, verifiedSingleDeviceTarget: false, blockers: [blocker] };
  }
  const keys = Object.keys(record);
  if (keys.length === 0) {
    return { references, verifiedSingleDeviceTarget: false, blockers: [] };
  }
  if (keys.length === 1 && keys[0] === "device") {
    const device = record.device;
    if (typeof device === "string" && knownDeviceIds.has(device)) {
      return { references, verifiedSingleDeviceTarget: true, blockers: [] };
    }
  }
  return { references, verifiedSingleDeviceTarget: false, blockers: [blocker] };
}

function semanticFromTextParts(parts: readonly unknown[]): AwningAction | null {
  const text = parts.filter((item): item is string => typeof item === "string")
    .join(" ")
    .toLowerCase();
  if (!text) return null;
  const normalized = ` ${text.replace(/[^a-z0-9åäö]+/g, " ")} `;
  const matches = new Set<AwningAction>();
  if (/ (open|up|raise|öppna|öppna upp|upp|höj|höja) /.test(normalized)) matches.add("OPEN");
  if (/ (stop|halt|stopp|stoppa) /.test(normalized)) matches.add("STOP");
  if (/ (close|down|lower|stäng|stänga|ner|sänk|sänka) /.test(normalized)) matches.add("CLOSE");
  return matches.size === 1 ? [...matches][0]! : null;
}

function semanticFromDescriptor(value: unknown): AwningAction | null {
  const descriptor = recordOf(value);
  if (!descriptor) return null;
  return semanticFromTextParts([descriptor.title, descriptor.titleFormatted, descriptor.hint]);
}

function enumValueSemantic(value: unknown): AwningAction | null {
  if (typeof value === "string") return semanticFromTextParts([value]);
  const record = recordOf(value);
  if (!record) return null;
  return semanticFromTextParts([record.id, record.value, record.title]);
}

function descriptorOwnerAlias(
  descriptor: Record<string, unknown> | undefined,
  registry: AliasRegistry,
  rawIds: Set<string>,
): string | null {
  if (!descriptor) return null;
  const owner = typeof descriptor.ownerId === "string" && descriptor.ownerId.length > 0
    ? descriptor.ownerId
    : typeof descriptor.ownerUri === "string" && descriptor.ownerUri.length > 0 ? descriptor.ownerUri : null;
  if (!owner) return null;
  rawIds.add(owner);
  return patch039AliasFor(registry, "owner", owner);
}

function makeBlockedUnknownSemantic(
  role: AwningRole,
  sourceKind: CandidateObservation["source_kind"],
  sourceAlias: string,
  sideEffectCount: number,
  extraBlockers: readonly string[],
): CandidateObservation[] {
  return AWNING_ACTIONS.map((action) => ({
    role,
    action,
    source_kind: sourceKind,
    source_alias: sourceAlias,
    structural_signature: `${sourceKind}:unresolved_semantic:v1`,
    blocking_reasons: ["UNRESOLVED_ACTION_SEMANTIC", ...extraBlockers].sort(),
    side_effect_device_count: sideEffectCount,
  }));
}

function optionsBlockCandidate(value: unknown): boolean {
  if (value === undefined || value === null) return false;
  if (Array.isArray(value)) return value.length > 0;
  const record = recordOf(value);
  if (record) return Object.keys(record).length > 0;
  return true;
}

function capabilityListBlockers(device: Record<string, unknown>, capabilitiesObj: Record<string, unknown> | null): string[] {
  if (device.capabilities === undefined) return [];
  if (!Array.isArray(device.capabilities)) return ["CAPABILITY_LIST_INCONSISTENT"];
  const seen = new Set<string>();
  let invalid = false;
  for (const item of device.capabilities) {
    if (typeof item !== "string" || item.length === 0 || seen.has(item)) {
      invalid = true;
      continue;
    }
    seen.add(item);
  }
  if (capabilitiesObj) {
    for (const capabilityId of seen) {
      if (!Object.hasOwn(capabilitiesObj, capabilityId) || !recordOf(capabilitiesObj[capabilityId])) invalid = true;
    }
    for (const capabilityId of Object.keys(capabilitiesObj)) {
      if (!seen.has(capabilityId)) invalid = true;
    }
  } else if (seen.size > 0) {
    invalid = true;
  }
  return invalid ? ["CAPABILITY_LIST_INCONSISTENT"] : [];
}

export interface NormalizedAwningEvidenceSources {
  devices: SanitizedDeviceEvidence[];
  flows: SanitizedFlowEvidence[];
  advancedFlows: SanitizedAdvancedFlowEvidence[];
  observations: CandidateObservation[];
  rawIds: string[];
  negativeEvidenceComplete: boolean;
}

export function normalizeAwningEvidenceSources(
  raw: RawAwningCollection,
  mapping: PrivateAwningMapping,
  registry: AliasRegistry,
): NormalizedAwningEvidenceSources {
  const rawIds = new Set<string>();
  const knownDeviceIds = new Set<string>();
  const rawDeviceById = new Map<string, Record<string, unknown>>();
  let negativeEvidenceComplete = true;

  for (const item of raw.devices) {
    const device = recordOf(item);
    if (!device) {
      negativeEvidenceComplete = false;
      continue;
    }
    const id = objectId(device);
    if (!id) {
      negativeEvidenceComplete = false;
      continue;
    }
    rawIds.add(id);
    knownDeviceIds.add(id);
    if (rawDeviceById.has(id)) negativeEvidenceComplete = false;
    rawDeviceById.set(id, device);
  }

  const roleByRawDeviceId = new Map<string, AwningRole>();
  for (const role of AWNING_ROLES) roleByRawDeviceId.set(mapping.mappings[role], role);

  const observations: CandidateObservation[] = [];
  const devices: SanitizedDeviceEvidence[] = [];
  for (const role of AWNING_ROLES) {
    const rawId = mapping.mappings[role];
    const device = rawDeviceById.get(rawId);
    if (!device) continue;
    const deviceAlias = patch039AliasFor(registry, "device", rawId);
    const driverRaw = typeof device.driverId === "string"
      ? device.driverId
      : typeof device.driver === "string" ? device.driver : null;
    if (driverRaw) rawIds.add(driverRaw);

    const capabilitiesObjRecord = recordOf(device.capabilitiesObj);
    if (device.capabilitiesObj !== undefined && !capabilitiesObjRecord) negativeEvidenceComplete = false;
    const listBlockers = capabilityListBlockers(device, capabilitiesObjRecord);
    if (listBlockers.length > 0) negativeEvidenceComplete = false;
    if (Array.isArray(device.capabilities)) {
      for (const capabilityId of device.capabilities) {
        if (typeof capabilityId === "string" && capabilityId.length > 0) rawIds.add(capabilityId);
      }
    }
    if (!capabilitiesObjRecord && Array.isArray(device.capabilities) && device.capabilities.length > 0) {
      negativeEvidenceComplete = false;
    }
    const capabilitiesObj = capabilitiesObjRecord ?? {};
    const capabilities: SanitizedCapabilityEvidence[] = [];
    for (const capabilityId of Object.keys(capabilitiesObj).sort()) {
      if (!capabilityId) {
        negativeEvidenceComplete = false;
        continue;
      }
      rawIds.add(capabilityId);
      const capability = recordOf(capabilitiesObj[capabilityId]);
      if (!capability) {
        negativeEvidenceComplete = false;
        continue;
      }
      const capabilityAlias = patch039AliasFor(registry, "capability", `${rawId}\u0000${capabilityId}`);
      const capabilityUnknown = unknownFields(capability, CAPABILITY_ALLOWED_FIELDS);
      const optionBlocker = optionsBlockCandidate(capability.options);
      const valuesMalformed = capability.values !== undefined && !Array.isArray(capability.values);
      if (capabilityUnknown.length > 0 || optionBlocker || valuesMalformed) negativeEvidenceComplete = false;
      const observedMetadataKeys = Object.keys(capability).filter((key) => CAPABILITY_METADATA_KEYS.has(key)).sort();
      const evidence = valueEvidence(capability.value);
      capabilities.push({
        capability_alias: capabilityAlias,
        ...evidence,
        last_updated_present: typeof capability.lastUpdated === "string" || typeof capability.lastUpdated === "number",
        observed_metadata_keys: observedMetadataKeys,
      });

      const blockers: string[] = [
        ...listBlockers,
        ...(capabilityUnknown.length > 0 ? ["CAPABILITY_UNKNOWN_FIELDS"] : []),
        ...(optionBlocker ? ["CAPABILITY_OPTIONS_UNINSPECTED"] : []),
        ...(valuesMalformed ? ["CAPABILITY_VALUES_MALFORMED"] : []),
      ];
      const semantics: AwningAction[] = [];
      if (Array.isArray(capability.values)) {
        for (const item of capability.values) {
          const semantic = enumValueSemantic(item);
          if (semantic) semantics.push(semantic);
        }
      }
      if (semantics.length > 0) {
        if (capability.setable !== true) blockers.push("CAPABILITY_SETABILITY_NOT_VERIFIED");
        if (capability.type !== "enum") blockers.push("CAPABILITY_ENUM_TYPE_NOT_VERIFIED");
        for (const action of semantics) {
          observations.push({
            role,
            action,
            source_kind: "device_capability",
            source_alias: capabilityAlias,
            structural_signature: `device_capability:${action}:enum:v1`,
            blocking_reasons: [...new Set(blockers)].sort(),
            side_effect_device_count: 1,
          });
        }
      } else {
        const titleSemantic = semanticFromTextParts([capability.title]);
        if (titleSemantic && capability.setable === true) {
          observations.push({
            role,
            action: titleSemantic,
            source_kind: "device_capability",
            source_alias: capabilityAlias,
            structural_signature: `device_capability:${titleSemantic}:value_unknown:v1`,
            blocking_reasons: [...new Set(["CAPABILITY_ACTION_VALUE_NOT_VERIFIED", ...blockers])].sort(),
            side_effect_device_count: 1,
          });
        }
      }
    }

    const classValue = typeof device.class === "string" && SAFE_DEVICE_CLASSES.has(device.class)
      ? device.class
      : null;
    devices.push({
      role,
      device_alias: deviceAlias,
      available: booleanOrNull(device.available),
      class: classValue,
      driver_alias: driverRaw ? patch039AliasFor(registry, "driver", driverRaw) : null,
      capabilities,
    });
  }

  const descriptorById = new Map<string, Record<string, unknown>>();
  const descriptorUnknownById = new Map<string, boolean>();
  for (const item of raw.flowCardActions) {
    const descriptor = recordOf(item);
    if (!descriptor) {
      negativeEvidenceComplete = false;
      continue;
    }
    const id = objectId(descriptor);
    if (!id) {
      negativeEvidenceComplete = false;
      continue;
    }
    rawIds.add(id);
    const unknown = unknownFields(descriptor, FLOW_CARD_DESCRIPTOR_ALLOWED_FIELDS);
    const malformedArgsDefinition = descriptor.args !== undefined && !Array.isArray(descriptor.args);
    if (unknown.length > 0 || malformedArgsDefinition) negativeEvidenceComplete = false;
    descriptorUnknownById.set(id, unknown.length > 0 || malformedArgsDefinition);
    descriptorOwnerAlias(descriptor, registry, rawIds);
    descriptorById.set(id, descriptor);
  }

  const flows: SanitizedFlowEvidence[] = [];
  const flowAllowed = new Set(["id", "_id", "name", "folder", "triggerable", "enabled", "trigger", "conditions", "actions"]);
  const flowActionAllowed = new Set(["id", "args", "droptoken"]);
  for (const item of raw.flows) {
    const flow = recordOf(item);
    if (!flow) {
      negativeEvidenceComplete = false;
      continue;
    }
    const flowId = objectId(flow);
    if (!flowId) {
      negativeEvidenceComplete = false;
      continue;
    }
    rawIds.add(flowId);
    const flowAlias = patch039AliasFor(registry, "flow", flowId);
    const topUnknown = unknownFields(flow, flowAllowed);
    if (topUnknown.length > 0) negativeEvidenceComplete = false;
    if (flow.actions !== undefined && !Array.isArray(flow.actions)) negativeEvidenceComplete = false;
    const rawActions = Array.isArray(flow.actions) ? flow.actions : [];
    const actionInspections: Array<SavedArgsInspection | null> = [];
    const flowRefs = new Set<string>();
    for (const rawAction of rawActions) {
      const action = recordOf(rawAction);
      if (!action) {
        negativeEvidenceComplete = false;
        actionInspections.push(null);
        continue;
      }
      const inspection = inspectSavedArgs(action.args, knownDeviceIds, "FLOW_ARGS_UNVERIFIED");
      actionInspections.push(inspection);
      for (const ref of inspection.references) flowRefs.add(ref);
      if (inspection.blockers.length > 0) negativeEvidenceComplete = false;
    }
    const sanitizedActions: SanitizedFlowEvidence["actions"] = [];
    for (let index = 0; index < rawActions.length; index += 1) {
      const rawAction = rawActions[index];
      const action = recordOf(rawAction);
      if (!action) continue;
      const inspection = actionInspections[index] ?? inspectSavedArgs(action.args, knownDeviceIds, "FLOW_ARGS_UNVERIFIED");
      const cardId = typeof action.id === "string" && action.id.length > 0 ? action.id : null;
      if (cardId) rawIds.add(cardId);
      const cardAlias = cardId
        ? patch039AliasFor(registry, "flow_card", cardId)
        : patch039AliasFor(registry, "flow_card", `${flowId}:missing-card`);
      const targetRoles = [...inspection.references]
        .map((id) => roleByRawDeviceId.get(id))
        .filter((role): role is AwningRole => role !== undefined)
        .sort();
      const actionUnknown = unknownFields(action, flowActionAllowed);
      if (actionUnknown.length > 0) negativeEvidenceComplete = false;
      const descriptor = cardId ? descriptorById.get(cardId) : undefined;
      sanitizedActions.push({
        card_alias: cardAlias,
        owner_alias: descriptorOwnerAlias(descriptor, registry, rawIds),
        target_roles: targetRoles,
        referenced_device_count: inspection.references.size,
        unknown_field_names: actionUnknown,
      });
      if (targetRoles.length === 0) continue;
      const semantic = semanticFromDescriptor(descriptor);
      const blockers = [
        ...(topUnknown.length > 0 ? ["FLOW_UNKNOWN_FIELDS"] : []),
        ...(actionUnknown.length > 0 ? ["FLOW_ACTION_UNKNOWN_FIELDS"] : []),
        ...inspection.blockers,
        ...(!inspection.verifiedSingleDeviceTarget ? ["FLOW_ARGS_TARGET_NOT_EXACT"] : []),
        ...(cardId && descriptorUnknownById.get(cardId) ? ["FLOW_CARD_DESCRIPTOR_UNKNOWN_FIELDS"] : []),
      ];
      const sideEffectCount = Math.max(flowRefs.size, rawActions.length > 1 ? 2 : 1);
      for (const role of targetRoles) {
        if (!semantic) {
          observations.push(...makeBlockedUnknownSemantic(role, "flow", flowAlias, sideEffectCount, blockers));
          continue;
        }
        observations.push({
          role,
          action: semantic,
          source_kind: "flow",
          source_alias: flowAlias,
          structural_signature: `flow:${semantic}:targeted_action:v2`,
          blocking_reasons: [...new Set(blockers)].sort(),
          side_effect_device_count: sideEffectCount,
        });
      }
    }
    flows.push({
      flow_alias: flowAlias,
      enabled: booleanOrNull(flow.enabled),
      triggerable: booleanOrNull(flow.triggerable),
      action_count: rawActions.length,
      actions: sanitizedActions,
    });
  }

  const advancedFlows: SanitizedAdvancedFlowEvidence[] = [];
  const advancedAllowed = new Set(["id", "_id", "name", "folder", "triggerable", "enabled", "cards"]);
  const advancedCardAllowed = new Set(["id", "ownerUri", "type", "x", "y", "args", "droptoken"]);
  for (const item of raw.advancedFlows) {
    const advanced = recordOf(item);
    if (!advanced) {
      negativeEvidenceComplete = false;
      continue;
    }
    const advancedId = objectId(advanced);
    if (!advancedId) {
      negativeEvidenceComplete = false;
      continue;
    }
    rawIds.add(advancedId);
    const advancedAlias = patch039AliasFor(registry, "advanced_flow", advancedId);
    const topUnknown = unknownFields(advanced, advancedAllowed);
    if (topUnknown.length > 0) negativeEvidenceComplete = false;
    const cardsRecord = recordOf(advanced.cards);
    if (advanced.cards !== undefined && !cardsRecord) negativeEvidenceComplete = false;
    const cardsObject = cardsRecord ?? {};
    const actionCards = Object.entries(cardsObject).filter(([, value]) => recordOf(value)?.type === "action");
    const flowRefs = new Set<string>();
    const inspectionByCardKey = new Map<string, SavedArgsInspection>();
    for (const [cardKey, value] of actionCards) {
      const card = recordOf(value)!;
      const inspection = inspectSavedArgs(card.args, knownDeviceIds, "ADVANCED_FLOW_ARGS_UNVERIFIED");
      inspectionByCardKey.set(cardKey, inspection);
      for (const ref of inspection.references) flowRefs.add(ref);
      if (inspection.blockers.length > 0) negativeEvidenceComplete = false;
    }
    const cards: SanitizedAdvancedFlowEvidence["cards"] = [];
    for (const [cardKey, rawCard] of Object.entries(cardsObject)) {
      const card = recordOf(rawCard);
      if (!card) {
        negativeEvidenceComplete = false;
        continue;
      }
      rawIds.add(cardKey);
      const descriptorId = typeof card.id === "string" && card.id.length > 0 ? card.id : null;
      if (descriptorId) rawIds.add(descriptorId);
      const cardAlias = patch039AliasFor(registry, "advanced_flow_card", `${advancedId}\u0000${cardKey}`);
      const cardType = card.type === "action" || card.type === "condition" || card.type === "trigger" ? card.type : "unknown";
      if (cardType === "unknown") negativeEvidenceComplete = false;
      const inspection = cardType === "action"
        ? inspectionByCardKey.get(cardKey) ?? inspectSavedArgs(card.args, knownDeviceIds, "ADVANCED_FLOW_ARGS_UNVERIFIED")
        : { references: findDeviceReferences(card.args, knownDeviceIds), verifiedSingleDeviceTarget: false, blockers: [] };
      const targetRoles = [...inspection.references]
        .map((id) => roleByRawDeviceId.get(id))
        .filter((role): role is AwningRole => role !== undefined)
        .sort();
      const cardUnknown = unknownFields(card, advancedCardAllowed);
      if (cardUnknown.length > 0) negativeEvidenceComplete = false;
      const descriptor = descriptorId ? descriptorById.get(descriptorId) : undefined;
      const cardOwner = typeof card.ownerUri === "string" && card.ownerUri.length > 0 ? card.ownerUri : null;
      if (cardOwner) rawIds.add(cardOwner);
      cards.push({
        card_alias: cardAlias,
        owner_alias: descriptorOwnerAlias(descriptor, registry, rawIds)
          ?? (cardOwner ? patch039AliasFor(registry, "owner", cardOwner) : null),
        type: cardType,
        target_roles: targetRoles,
        referenced_device_count: inspection.references.size,
        unknown_field_names: cardUnknown,
      });
      if (cardType !== "action" || targetRoles.length === 0) continue;
      const semantic = semanticFromDescriptor(descriptor);
      const blockers = [
        ...(topUnknown.length > 0 ? ["ADVANCED_FLOW_UNKNOWN_FIELDS"] : []),
        ...(cardUnknown.length > 0 ? ["ADVANCED_FLOW_CARD_UNKNOWN_FIELDS"] : []),
        ...inspection.blockers,
        ...(!inspection.verifiedSingleDeviceTarget ? ["ADVANCED_FLOW_ARGS_TARGET_NOT_EXACT"] : []),
        ...(descriptorId && descriptorUnknownById.get(descriptorId) ? ["FLOW_CARD_DESCRIPTOR_UNKNOWN_FIELDS"] : []),
      ];
      const sideEffectCount = Math.max(flowRefs.size, actionCards.length > 1 ? 2 : 1);
      for (const role of targetRoles) {
        if (!semantic) {
          observations.push(...makeBlockedUnknownSemantic(role, "advanced_flow", advancedAlias, sideEffectCount, blockers));
          continue;
        }
        observations.push({
          role,
          action: semantic,
          source_kind: "advanced_flow",
          source_alias: advancedAlias,
          structural_signature: `advanced_flow:${semantic}:targeted_action:v2`,
          blocking_reasons: [...new Set(blockers)].sort(),
          side_effect_device_count: sideEffectCount,
        });
      }
    }
    advancedFlows.push({
      advanced_flow_alias: advancedAlias,
      enabled: booleanOrNull(advanced.enabled),
      triggerable: booleanOrNull(advanced.triggerable),
      card_count: Object.keys(cardsObject).length,
      cards,
    });
  }

  devices.sort((a, b) => a.role.localeCompare(b.role));
  flows.sort((a, b) => a.flow_alias.localeCompare(b.flow_alias));
  advancedFlows.sort((a, b) => a.advanced_flow_alias.localeCompare(b.advanced_flow_alias));
  observations.sort((a, b) => `${a.role}.${a.action}.${a.source_alias}`.localeCompare(`${b.role}.${b.action}.${b.source_alias}`));
  return {
    devices,
    flows,
    advancedFlows,
    observations,
    rawIds: [...rawIds].sort(),
    negativeEvidenceComplete,
  };
}
