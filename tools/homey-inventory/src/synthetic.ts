import type { CandidateConnector, ReadonlyHomeyClient } from "./readonly-client.js";
import type { CandidateKind, RawInventory } from "./model.js";

export const SYNTHETIC_INVENTORY: RawInventory = {
  platform: { name: "Synthetic Homey", version: "0.0-test" },
  zones: [
    { id: "raw-zone-2", name: "Upstairs", parent: "raw-zone-1" },
    { id: "raw-zone-1", name: "Home", parent: null },
  ],
  devices: [
    {
      id: "raw-device-1",
      name: "Synthetic Lamp",
      zone: "raw-zone-2",
      class: "light",
      driver: "synthetic:light",
      capabilities: [
        { id: "onoff", readable: true, writable: true, valueType: "boolean" },
      ],
    },
  ],
  flows: [{ id: "raw-flow-1", name: "Synthetic Flow" }],
  advancedFlows: [{ id: "raw-advanced-flow-1", name: "Synthetic Advanced Flow" }],
  moods: [{ id: "raw-mood-1", name: "Synthetic Mood" }],
};

class SyntheticClient implements ReadonlyHomeyClient {
  async collect(): Promise<RawInventory> {
    return structuredClone(SYNTHETIC_INVENTORY);
  }
}

export class SyntheticConnector implements CandidateConnector {
  constructor(public readonly kind: CandidateKind = "LOCAL_SECURE") {}
  async connect(_timeoutMs: number): Promise<ReadonlyHomeyClient> {
    return new SyntheticClient();
  }
}
