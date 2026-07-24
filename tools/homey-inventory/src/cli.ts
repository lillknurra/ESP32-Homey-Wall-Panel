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
import { loadPrivateLiveConfig } from "./private-config.js";

interface Options {
  connection: ConnectionMode;
  output: string;
  timeout: number;
  aliasRegistry: string;
  synthetic: boolean;
  live: boolean;
  privateConfig: string;
}

function parseArgs(argv: string[]): Options {
  const options: Options = {
    connection: "auto",
    output: "",
    timeout: 10_000,
    aliasRegistry: "",
    synthetic: false,
    live: false,
    privateConfig: "",
  };

  for (let index = 0; index < argv.length; index += 1) {
    const arg = argv[index];
    if (arg === "--synthetic") {
      options.synthetic = true;
      continue;
    }
    if (arg === "--live") {
      options.live = true;
      continue;
    }

    const value = argv[index + 1];
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
    } else if (arg === "--private-config") {
      options.privateConfig = resolve(value);
    } else {
      throw new Error(`Unknown argument ${arg}`);
    }
    index += 1;
  }

  if (!options.output) throw new Error("--output is required");
  if (!options.aliasRegistry) throw new Error("--alias-registry is required");
  if (!Number.isInteger(options.timeout) || options.timeout < 100) {
    throw new Error("--timeout must be an integer >= 100");
  }
  if (options.synthetic && options.live) throw new Error("--synthetic and --live are mutually exclusive");
  if (!options.synthetic && !options.live) throw new Error("Non-synthetic execution requires explicit --live");
  if (options.live && !options.privateConfig) throw new Error("--live requires --private-config");
  return options;
}

async function main(): Promise<void> {
  const options = parseArgs(process.argv.slice(2));
  const repositoryRoot = resolve(import.meta.dirname, "../../..");
  const privateConfig = options.live
    ? await loadPrivateLiveConfig(options.privateConfig, repositoryRoot)
    : undefined;

  const connectors = options.synthetic
    ? [new SyntheticConnector()]
    : [
        new HomeyApiConnector("LOCAL_SECURE", { liveEnabled: true, privateConfig }),
        new HomeyApiConnector("LOCAL", { liveEnabled: true, privateConfig }),
        new HomeyApiConnector("MDNS", { liveEnabled: true, privateConfig }),
        new HomeyApiConnector("REMOTE_FORWARDED", { liveEnabled: true, privateConfig }),
        new HomeyApiConnector("CLOUD", { liveEnabled: true, privateConfig }),
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
