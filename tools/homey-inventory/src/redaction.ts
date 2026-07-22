const SECRET_PATTERNS: RegExp[] = [
  /bearer\s+[a-z0-9._~+\/-]+=*/gi,
  /(?:access|refresh|personal)[_-]?token["'\s:=]+[^\s,"']+/gi,
  /\b(?:\d{1,3}\.){3}\d{1,3}\b/g,
  /\b[0-9a-f]{2}(?::[0-9a-f]{2}){5}\b/gi,
  /https?:\/\/[^\s]+/gi,
];

export function redact(value: string): string {
  return SECRET_PATTERNS.reduce((current, pattern) => current.replace(pattern, "[REDACTED]"), value);
}

export function assertSanitized(serialized: string, rawIds: readonly string[]): void {
  for (const rawId of rawIds) {
    if (rawId && serialized.includes(rawId)) throw new Error("Raw Homey ID leaked into sanitized output");
  }
  if (redact(serialized) !== serialized) throw new Error("Sensitive value pattern detected in sanitized output");
}
