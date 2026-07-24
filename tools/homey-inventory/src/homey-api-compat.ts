import { CandidateError } from "./errors.js";
import type { AllowlistedLiveSource } from "./live-collector.js";

export interface HomeyApiReadSurface {
  platform: { getSystemInfo(): Promise<unknown> };
  zones: { getZones(): Promise<unknown> };
  devices: { getDevices(): Promise<unknown> };
  flow: { getFlows(): Promise<unknown>; getAdvancedFlows(): Promise<unknown> };
  moods: { getMoods(): Promise<unknown> };
}

export function assertCompatibleReadSurface(value: unknown): asserts value is HomeyApiReadSurface {
  const v = value as HomeyApiReadSurface;
  const methods = [v?.platform?.getSystemInfo, v?.zones?.getZones, v?.devices?.getDevices,
    v?.flow?.getFlows, v?.flow?.getAdvancedFlows, v?.moods?.getMoods];
  if (methods.some((method) => typeof method !== "function")) {
    throw new CandidateError("API_INCOMPATIBILITY", "Homey API read surface is incompatible with the pinned adapter contract");
  }
}

export type HomeyApiSourceFactory = (surface: HomeyApiReadSurface) => AllowlistedLiveSource;
