import { patch039AliasFor, type AliasRegistry } from "./aliases.js";
import { CandidateError } from "./errors.js";
import { AWNING_ROLES, type AwningRole } from "./awning-model.js";
import type { PrivateAwningMapping } from "./awning-private-mapping.js";

export interface PrivateOperatorCandidate {
  device_alias: string;
  display_name: string | null;
  driver_alias: string | null;
  capability_aliases: string[];
}

export interface PrivateOperatorCandidateSet {
  candidates: PrivateOperatorCandidate[];
  privateCorrelation: ReadonlyMap<string, string>;
}

function recordOf(value: unknown): Record<string, unknown> | null {
  return value && typeof value === "object" && !Array.isArray(value) ? value as Record<string, unknown> : null;
}

function rawDeviceId(device: Record<string, unknown>): string | null {
  const value = device.id ?? device._id;
  return typeof value === "string" && value.length > 0 ? value : null;
}

function rawCapabilityIds(device: Record<string, unknown>): string[] {
  const result = new Set<string>();
  if (Array.isArray(device.capabilities)) {
    for (const value of device.capabilities) if (typeof value === "string" && value.length > 0) result.add(value);
  }
  const obj = recordOf(device.capabilitiesObj);
  if (obj) for (const key of Object.keys(obj)) if (key.length > 0) result.add(key);
  return [...result].sort();
}

export function buildPrivateOperatorCandidates(
  rawDevices: readonly unknown[],
  registry: AliasRegistry,
): PrivateOperatorCandidateSet {
  const candidates: PrivateOperatorCandidate[] = [];
  const correlation = new Map<string, string>();
  for (const raw of rawDevices) {
    const device = recordOf(raw);
    if (!device) continue;
    const id = rawDeviceId(device);
    if (!id) continue;
    const deviceAlias = patch039AliasFor(registry, "device", id);
    if (correlation.has(deviceAlias)) {
      throw new CandidateError("SCHEMA_MISMATCH", "Patch039 device alias collision");
    }
    correlation.set(deviceAlias, id);
    const driver = typeof device.driverId === "string"
      ? device.driverId
      : typeof device.driver === "string" ? device.driver : null;
    const capabilityAliases = rawCapabilityIds(device).map((capabilityId) =>
      patch039AliasFor(registry, "capability", `${id}\u0000${capabilityId}`));
    candidates.push({
      device_alias: deviceAlias,
      display_name: typeof device.name === "string" ? device.name : null,
      driver_alias: driver ? patch039AliasFor(registry, "driver", driver) : null,
      capability_aliases: capabilityAliases,
    });
  }
  candidates.sort((a, b) => a.device_alias.localeCompare(b.device_alias));
  return { candidates, privateCorrelation: correlation };
}

export function createPrivateAwningMappingFromSelection(input: {
  selections: Record<AwningRole, string>;
  privateCorrelation: ReadonlyMap<string, string>;
  selectedHomeyDigest: string;
  generation: number;
}): PrivateAwningMapping {
  if (!Number.isInteger(input.generation) || input.generation <= 0) {
    throw new CandidateError("CONFIGURATION", "Patch039 mapping generation must be positive");
  }
  const mappings = {} as Record<AwningRole, string>;
  const selectedRawIds = new Set<string>();
  for (const role of AWNING_ROLES) {
    const alias = input.selections[role];
    const rawId = input.privateCorrelation.get(alias);
    if (!rawId) throw new CandidateError("CONFIGURATION", "Patch039 operator selection references an unknown sanitized candidate");
    if (selectedRawIds.has(rawId)) throw new CandidateError("CONFIGURATION", "Patch039 operator selection must choose three unique devices");
    selectedRawIds.add(rawId);
    mappings[role] = rawId;
  }
  return {
    schema_version: 1,
    purpose: "read_only_awning_evidence",
    generation: input.generation,
    selected_homey_id_sha256: input.selectedHomeyDigest,
    mappings,
  };
}
