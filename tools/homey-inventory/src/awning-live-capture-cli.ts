#!/usr/bin/env node
import { isAbsolute, resolve } from "node:path";
import { AWNING_ROLES, type AwningRole } from "./awning-model.js";
import {
  inspectPatch042AwningCandidates,
  runPatch042AwningCapture,
  savePatch042AwningMapping,
} from "./awning-live-capture.js";
import { redact } from "./redaction.js";

type Command = "inspect" | "map" | "capture";

interface ParsedOptions {
  command: Command;
  privateConfigPath: string;
  mappingPath?: string;
  outputDir?: string;
  generation?: number;
  selections?: Record<AwningRole, string>;
}

function requiredAbsolute(value: string | undefined, flag: string): string {
  if (!value) throw new Error(`${flag} is required`);
  if (!isAbsolute(value)) throw new Error(`${flag} must be an absolute path`);
  return value;
}

function parseArgs(argv: string[]): ParsedOptions {
  const command = argv[0];
  if (command !== "inspect" && command !== "map" && command !== "capture") {
    throw new Error("First argument must be inspect, map, or capture");
  }

  const values = new Map<string, string>();
  for (let index = 1; index < argv.length; index += 2) {
    const flag = argv[index];
    const value = argv[index + 1];
    if (!flag || !flag.startsWith("--") || !value) {
      throw new Error("Patch042 options must be --flag value pairs");
    }
    if (values.has(flag)) throw new Error(`Duplicate option ${flag}`);
    values.set(flag, value);
  }

  const allowed = new Set(["--private-config"]);
  if (command === "map") {
    for (const flag of ["--mapping", "--generation", "--awning-1", "--awning-2", "--awning-3"]) allowed.add(flag);
  }
  if (command === "capture") {
    allowed.add("--mapping");
    allowed.add("--output");
  }
  for (const flag of values.keys()) {
    if (!allowed.has(flag)) throw new Error(`Unknown option ${flag} for ${command}`);
  }

  const privateConfigPath = requiredAbsolute(values.get("--private-config"), "--private-config");
  if (command === "inspect") return { command, privateConfigPath };

  const mappingPath = requiredAbsolute(values.get("--mapping"), "--mapping");
  if (commmand === "capture") {
    return {
      command,
      privateConfigPath,
      mappingPath,
      outputDir: requiredAbsolute(values.get("--output"), "--output"),
    };
  }

  const generation = Number(values.get("--generation"));
  if (!Number.isInteger(generation) || generation <= 0) {
    throw new Error("--generation must be a positive integer");
  }
  const selections = {} as Record<AwningRole, string>;
  for (const role of AWNING_ROLES) {
    const alias = values.get(`--${role.replace("_", "-")}`);
    if (!alias || !/^device_[0-9a-f]{12}$/.test(alias)) {
      throw new Error(`--${role.replace("_", "-")} must be a sanitized device alias`);
    }
    selections[role] = alias;
  }
  return { command, privateConfigPath, mappingPath, generation, selections };
}

async function main(): Promise<void> {
  const options = parseArgs(process.argv.slice(2));
  const repositoryRoot = resolve(import.meta.dirname, "../../../..");
  let result: unknown;
  if (options.command === "inspect") {
    result = await inspectPatch042AwningCandidates({
      privateConfigPath: options.privateConfigPath,
      repositoryRoot,
    });
  } else if (options.command === "map") {
    result = await savePatch042AwningMapping({
      privateConfigPath: options.privateConfigPath,
      mappingPath: options.mappingPath!,
      repositoryRoot,
      generation: options.generation!,
      selections: options.selections!,
    });
  } else {
    result = await runPatch042AwningCapture({
      privateConfigPath: options.privateConfigPath,
      mappingPath: options.mappingPath!,
      outputDir: options.outputDir!,
      repositoryRoot,
    });
  }
  console.log(JSON.stringify(result, null, 2));
}

main().catch((error: unknown) => {
  const message = error instanceof Error ? error.message : String(error);
  console.error(`FAIL: ${redact(message)}`);
  process.exitCode = 1;
});
