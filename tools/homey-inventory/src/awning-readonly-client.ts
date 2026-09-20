import type { Patch039CallLedger } from "./awning-call-ledger.js";
import type { Patch039SessionEvidence, RawAwningCollection } from "./awning-model.js";

export const PATCH039_ALLOWED_HOMEY_METHODS = [
  "ManagerDevices.getDevices",
  "ManagerDevices.getCapabilityValue",
  "ManagerFlow.getFlows",
  "ManagerFlow.getFlowCardActions",
  "ManagerFlow.getAdvancedFlows",
] as const;

export const PATCH039_FORBIDDEN_HOMEY_METHOD_NAMES = [
  "setCapabilityValue",
  "runFlowCardAction",
  "triggerFlow",
  "testFlow",
  "triggerAdvancedFlow",
  "activateMood",
  "createFlow",
  "updateFlow",
  "deleteFlow",
  "createAdvancedFlow",
  "updateAdvancedFlow",
  "deleteAdvancedFlow",
  "create",
  "update",
  "delete",
  "pair",
  "unpair",
  "genericApiCall",
] as const;

export interface Patch039ReadonlyHomeyClient {
  getDevices(): Promise<unknown[]>;
  getFlows(): Promise<unknown[]>;
  getFlowCardActions(): Promise<unknown[]>;
  getAdvancedFlows(): Promise<unknown[]>;
  getCapabilityValue(deviceId: string, capabilityId: string): Promise<unknown>;
  sessionEvidence(): Patch039SessionEvidence;
  readonly ledger: Patch039CallLedger;
}

export async function collectPatch039ReadSurface(
  client: Patch039ReadonlyHomeyClient,
): Promise<RawAwningCollection> {
  const devices = await client.getDevices();
  const flows = await client.getFlows();
  const flowCardActions = await client.getFlowCardActions();
  const advancedFlows = await client.getAdvancedFlows();
  return { devices, flows, flowCardActions, advancedFlows };
}
