#!/usr/bin/env node
import { resolve } from "node:path";
import { loadRegistry, saveRegistry } from "./aliases.js";
import { selectConnection } from "./connection-selector.js";
import { HomeyApiConnector } from "./homey-api-adapter.js";
import type { ConnectionMode } from "./model.js";
import { normalize } from "./normalizer.js";
import { publishCandidate } from "./publication.js";
import { redact } from "./redaction.js";
import { SyntheticConnector } from "./synthetic.js";

interface Options {
  connection: ConnectionMode;
  output: string;
  timeout: number;
  aliasRegistry: string;
  synthetic: boolean;
}

function parseArgs(argv: string[]): Options {
  const options: Options = {
    connection: "auto",
    output: "",
    timeout: 10_000,
    aliasRegistry: "",
    synthetic: false,
  };

  for (let index = 0; index < argv.length; index += 1) {
    const arg = argv[index];
    const value = argv[index + 1];
    if (arg === "--synthetic") {
      options.synthetic = true;
      continue;
    }
    if (!value) throw new Error(`Missing value for ${arg}`);
    if (arg === "--connection") {
      if (!["auto", "local", "cloud"].includes(value)) throw new Error("Invalid --connection");
      options.connection = value as ConnectionMode;
    } else if (arg === "--output") {
      options.output = resolve(value);
    } else if (arg === "--timeout") {
      options.timeout = Number(value);
    } else if (arg === "--alias-registry") {
      options.aliasRegistry = resolve(value);
    } else {
      throw new Error(`Unknown argument ${arg}`);
    }
    index += 1;
  }

  if (!options.output) throw new Error("--output is required");
  if (!options.aliasRegistry) throw new Error("--alias-registry is required");
  if (!Number.isInteger(options.timeout) || options.timeout < 100) throw new Error("--timeout must be an integer >= 100");
  return options;
}

async function main(): Promise<void> {
  const options = parseArgs(process.argv.slice(2));
  const connectors = options.synthetic
    ? [new SyntheticConnector()]
    : [
        new HomeyApiConnector("LOCAL_SECURE"),
        new HomeyApiConnector("LOCAL"),
        new HomeyApiConnector("MDNS"),
        new HomeyApiConnector("REMOTE_FORWARDED"),
        new HomeyApiConnector("CLOUD"),
      ];

  const selected = await selectConnection(options.connection, connectors, options.timeout);
  const raw = await selected.client.collect();
  const registry = await loadRegistry(options.aliasRegistry);
  const normalized = normalize(
    raw,
    registry,
    {
      requested: options.connection,
      selected: selected.selected,
      fallbackCount: selected.fallbackCount,
    },
    new Date().toISOString(),
  );
  await publishCandidate(options.output, normalized.inventory, normalized.rawIds);
  await saveRegistry(options.aliasRegistry, registry);
  console.log("PASS: sanitized candidate inventory published");
}

main().catch((error: unknown) => {
  const message = error instanceof Error ? error.message : String(error);
  console.error(`FAIL: ${redact(message)}`);
  process.exitCode = 1;
});
