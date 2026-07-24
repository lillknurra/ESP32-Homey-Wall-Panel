import type { RawInventory } from "./model.js";
import { SanitizedCallLedger } from "./call-ledger.js";

export interface AllowlistedLiveSource {
  readPlatformMetadata(): Promise<RawInventory["platform"]>;
  readZones(): Promise<RawInventory["zones"]>;
  readDevices(): Promise<RawInventory["devices"]>;
  readFlows(): Promise<RawInventory["flows"]>;
  readAdvancedFlows(): Promise<RawInventory["advancedFlows"]>;
  readMoods(): Promise<RawInventory["moods"]>;
}

export class AllowlistedLiveCollector {
  constructor(
    private readonly source: AllowlistedLiveSource,
    private readonly ledger: SanitizedCallLedger,
  ) {}

  async collect(): Promise<RawInventory> {
    this.ledger.record("platform_metadata.read");
    const platform = await this.source.readPlatformMetadata();
    this.ledger.record("zones.read");
    const zones = await this.source.readZones();
    this.ledger.record("devices.read");
    const devices = await this.source.readDevices();
    this.ledger.record("flows.read");
    const flows = await this.source.readFlows();
    this.ledger.record("advanced_flows.read");
    const advancedFlows = await this.source.readAdvancedFlows();
    this.ledger.record("moods.read");
    const moods = await this.source.readMoods();
    return { platform, zones, devices, flows, advancedFlows, moods };
  }
}
