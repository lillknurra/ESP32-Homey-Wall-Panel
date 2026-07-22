import { CandidateError, permitsFallback } from "./errors.js";
import type { CandidateConnector, ReadonlyHomeyClient } from "./readonly-client.js";
import type { CandidateKind, ConnectionMode } from "./model.js";

const AUTO_ORDER: CandidateKind[] = [
  "LOCAL_SECURE",
  "LOCAL",
  "MDNS",
  "REMOTE_FORWARDED",
  "CLOUD",
];

function allowed(mode: ConnectionMode, kind: CandidateKind): boolean {
  if (mode === "auto") return true;
  if (mode === "cloud") return kind === "CLOUD";
  return kind !== "CLOUD";
}

export async function selectConnection(
  mode: ConnectionMode,
  connectors: readonly CandidateConnector[],
  timeoutMs: number,
): Promise<{ client: ReadonlyHomeyClient; selected: CandidateKind; fallbackCount: number }> {
  const byKind = new Map(connectors.map((connector) => [connector.kind, connector]));
  const order = AUTO_ORDER.filter((kind) => allowed(mode, kind));
  let fallbackCount = 0;

  for (const kind of order) {
    const connector = byKind.get(kind);
    if (!connector) continue;
    try {
      return { client: await connector.connect(timeoutMs), selected: kind, fallbackCount };
    } catch (error) {
      if (!permitsFallback(error)) throw error;
      fallbackCount += 1;
    }
  }

  throw new CandidateError("REACHABILITY", `No reachable candidate for connection mode ${mode}`);
}
