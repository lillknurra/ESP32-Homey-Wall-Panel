import { createHash } from "node:crypto";
import { readFile, writeFile } from "node:fs/promises";

export type AliasKind =
  | "zone"
  | "device"
  | "driver"
  | "capability"
  | "flow"
  | "flow_card"
  | "advanced_flow"
  | "advanced_flow_card"
  | "owner"
  | "mood";
export type AliasRegistry = Record<string, string>;

export async function loadRegistry(path: string): Promise<AliasRegistry> {
  try {
    const parsed: unknown = JSON.parse(await readFile(path, "utf8"));
    if (!parsed || typeof parsed !== "object" || Array.isArray(parsed)) {
      throw new Error("Alias registry must be a JSON object");
    }
    return parsed as AliasRegistry;
  } catch (error: unknown) {
    if ((error as NodeJS.ErrnoException).code === "ENOENT") return {};
    throw error;
  }
}

export function aliasFor(registry: AliasRegistry, kind: AliasKind, rawId: string): string {
  const key = `${kind}:${rawId}`;
  const existing = registry[key];
  if (existing) return existing;
  const digest = createHash("sha256").update(key).digest("hex").slice(0, 12);
  const alias = `${kind}_${digest}`;
  registry[key] = alias;
  return alias;
}

export function patch039AliasFor(registry: AliasRegistry, kind: AliasKind, rawId: string): string {
  const key = `${kind}:${rawId}`;
  const alias = aliasFor(registry, kind, rawId);
  const expectedPattern = new RegExp(`^${kind}_[0-9a-f]{12}$`);
  if (!expectedPattern.test(alias)) {
    throw new Error("Patch039 alias registry contains a non-canonical alias");
  }
  for (const [otherKey, otherAlias] of Object.entries(registry)) {
    if (otherKey !== key && otherAlias === alias) {
      throw new Error("Patch039 alias registry contains an alias collision");
    }
  }
  return alias;
}

export async function saveRegistry(path: string, registry: AliasRegistry): Promise<void> {
  const ordered = Object.fromEntries(Object.entries(registry).sort(([a], [b]) => a.localeCompare(b)));
  await writeFile(path, `${JSON.stringify(ordered, null, 2)}\n`, { mode: 0o600 });
}
