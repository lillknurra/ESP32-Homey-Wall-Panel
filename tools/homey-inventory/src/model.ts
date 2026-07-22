export type ConnectionMode = "auto" | "local" | "cloud";
export type CandidateKind = "LOCAL_SECURE" | "LOCAL" | "MDNS" | "REMOTE_FORWARDED" | "CLOUD";
export type FailureClass =
  | "DISCOVERY"
  | "REACHABILITY"
  | "AUTHENTICATION"
  | "AUTHORIZATION"
  | "TLS"
  | "API_INCOMPATIBILITY"
  | "MALFORMED_RESPONSE"
  | "SCHEMA_MISMATCH"
  | "CONFIGURATION";

export interface RawZone { id: string; name: string; parent?: string | null; }
export interface RawCapability {
  id: string;
  readable: boolean;
  writable: boolean;
  valueType: string | null;
}
export interface RawDevice {
  id: string;
  name: string;
  zone?: string | null;
  class?: string | null;
  driver?: string | null;
  capabilities: RawCapability[];
}
export interface RawNamedObject { id: string; name: string; }
export interface RawInventory {
  platform: { name: string | null; version: string | null };
  zones: RawZone[];
  devices: RawDevice[];
  flows: RawNamedObject[];
  advancedFlows: RawNamedObject[];
  moods: RawNamedObject[];
}

export interface Inventory {
  schema_version: "1.0.0";
  generator: { name: "homey-inventory"; version: "0.4.0" };
  generation: { timestamp: string };
  connection: {
    requested: ConnectionMode;
    selected: CandidateKind;
    fallback_count: number;
  };
  summary: {
    zones: number;
    devices: number;
    flows: number;
    advanced_flows: number;
    moods: number;
  };
  platform: { name: string | null; version: string | null };
  zones: Array<{ alias: string; name: string; parent_alias: string | null }>;
  devices: Array<{
    alias: string;
    name: string;
    zone_alias: string | null;
    class: string | null;
    driver: string | null;
    capabilities: RawCapability[];
  }>;
  flows: Array<{ alias: string; name: string }>;
  advanced_flows: Array<{ alias: string; name: string }>;
  moods: Array<{ alias: string; name: string }>;
  warnings: string[];
  privacy_report: {
    raw_ids_present: false;
    secrets_present: false;
    redaction_applied: true;
  };
}
