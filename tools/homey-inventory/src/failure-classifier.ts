import type { FailureClass } from "./model.js";

export interface FailureLike { code?: unknown; statusCode?: unknown; message?: unknown; }
export function classifyLiveFailure(value: unknown): FailureClass {
  const e = (value && typeof value === "object" ? value : {}) as FailureLike;
  const code = String(e.code ?? "").toUpperCase();
  const status = Number(e.statusCode ?? 0);
  const message = String(e.message ?? value ?? "").toLowerCase();
  if (status === 401 || message.includes("unauthenticated") || message.includes("invalid token")) return "AUTHENTICATION";
  if (status === 403 || message.includes("forbidden") || message.includes("not authorized")) return "AUTHORIZATION";
  if (["CERT_HAS_EXPIRED", "DEPTH_ZERO_SELF_SIGNED_CERT", "UNABLE_TO_VERIFY_LEAF_SIGNATURE"].includes(code) || message.includes("tls")) return "TLS";
  if (["ENOTFOUND", "EAI_AGAIN"].includes(code) || message.includes("discovery")) return "DISCOVERY";
  if (["ECONNREFUSED", "ETIMEDOUT", "EHOSTUNREACH", "ENETUNREACH"].includes(code)) return "REACHABILITY";
  if (message.includes("unsupported api") || message.includes("missing method")) return "API_INCOMPATIBILITY";
  if (message.includes("malformed") || message.includes("invalid json")) return "MALFORMED_RESPONSE";
  return "CONFIGURATION";
}
