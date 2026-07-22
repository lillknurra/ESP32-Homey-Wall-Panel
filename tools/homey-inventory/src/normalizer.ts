import { aliasFor, type AliasRegistry } from "./aliases.js";
import type { CandidateKind, ConnectionMode, Inventory, RawInventory } from "./model.js";

const byAlias = <T extends { alias: string }>(a: T, b: T) => a.alias.localeCompare(b.alias);

export function normalize(
  raw: RawInventory,
  registry: AliasRegistry,
  connection: { requested: ConnectionMode; selected: CandidateKind; fallbackCount: number },
  timestamp: string,
): { inventory: Inventory; rawIds: string[] } {
  const rawIds: string[] = [];
  const zoneAliases = new Map<string, string>();

  for (const zone of raw.zones) {
    rawIds.push(zone.id);
    zoneAliases.set(zone.id, aliasFor(registry, "zone", zone.id));
  }

  const warnings: string[] = [];
  const zones = raw.zones.map((zone) => ({
    alias: zoneAliases.get(zone.id)!,
    name: zone.name,
    parent_alias: zone.parent ? zoneAliases.get(zone.parent) ?? null : null,
  })).sort(byAlias);

  for (const zone of raw.zones) {
    if (zone.parent && !zoneAliases.has(zone.parent)) {
      warnings.push(`Zone ${zoneAliases.get(zone.id)} references a missing parent alias`);
    }
  }

  const devices = raw.devices.map((device) => {
    rawIds.push(device.id);
    if (device.zone) rawIds.push(device.zone);
    const zoneAlias = device.zone ? zoneAliases.get(device.zone) ?? null : null;
    if (device.zone && !zoneAlias) warnings.push(`Device ${aliasFor(registry, "device", device.id)} references a missing zone alias`);
    return {
      alias: aliasFor(registry, "device", device.id),
      name: device.name,
      zone_alias: zoneAlias,
      class: device.class ?? null,
      driver: device.driver ?? null,
      capabilities: [...device.capabilities].sort((a, b) => a.id.localeCompare(b.id)),
    };
  }).sort(byAlias);

  const named = (
    entries: RawInventory["flows"],
    kind: "flow" | "advanced_flow" | "mood",
  ) => entries.map((entry) => {
    rawIds.push(entry.id);
    return { alias: aliasFor(registry, kind, entry.id), name: entry.name };
  }).sort(byAlias);

  const flows = named(raw.flows, "flow");
  const advancedFlows = named(raw.advancedFlows, "advanced_flow");
  const moods = named(raw.moods, "mood");

  return {
    rawIds,
    inventory: {
      schema_version: "1.0.0",
      generator: { name: "homey-inventory", version: "0.4.0" },
      generation: { timestamp },
      connection: {
        requested: connection.requested,
        selected: connection.selected,
        fallback_count: connection.fallbackCount,
      },
      summary: {
        zones: zones.length,
        devices: devices.length,
        flows: flows.length,
        advanced_flows: advancedFlows.length,
        moods: moods.length,
      },
      platform: raw.platform,
      zones,
      devices,
      flows,
      advanced_flows: advancedFlows,
      moods,
      warnings: warnings.sort(),
      privacy_report: {
        raw_ids_present: false,
        secrets_present: false,
        redaction_applied: true,
      },
    },
  };
}
