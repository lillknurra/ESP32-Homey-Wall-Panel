import { mkdir, rename, rm, writeFile } from "node:fs/promises";
import { dirname, join } from "node:path";
import type { Inventory } from "./model.js";
import { renderMarkdown } from "./markdown.js";
import { assertSanitized } from "./redaction.js";

export async function publishCandidate(
  outputDir: string,
  inventory: Inventory,
  rawIds: readonly string[],
): Promise<void> {
  await mkdir(outputDir, { recursive: true, mode: 0o700 });
  const serialized = `${JSON.stringify(inventory, null, 2)}\n`;
  assertSanitized(serialized, rawIds);
  const markdown = renderMarkdown(inventory);
  assertSanitized(markdown, rawIds);

  const nonce = `${process.pid}-${Date.now()}`;
  const jsonTmp = join(outputDir, `.homey_inventory.${nonce}.json.tmp`);
  const mdTmp = join(outputDir, `.homey_inventory.${nonce}.md.tmp`);
  const jsonFinal = join(outputDir, "homey_inventory.json");
  const mdFinal = join(outputDir, "homey_inventory.md");

  try {
    await writeFile(jsonTmp, serialized, { mode: 0o600 });
    await writeFile(mdTmp, markdown, { mode: 0o600 });
    await rename(jsonTmp, jsonFinal);
    await rename(mdTmp, mdFinal);
  } finally {
    await rm(jsonTmp, { force: true });
    await rm(mdTmp, { force: true });
  }
}
