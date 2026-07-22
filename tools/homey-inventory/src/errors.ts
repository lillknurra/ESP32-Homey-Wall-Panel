import type { FailureClass } from "./model.js";

export class CandidateError extends Error {
  constructor(
    public readonly failureClass: FailureClass,
    message: string,
    options?: { cause?: unknown },
  ) {
    super(message, options);
    this.name = "CandidateError";
  }
}

export function permitsFallback(error: unknown): boolean {
  return error instanceof CandidateError
    && (error.failureClass === "DISCOVERY" || error.failureClass === "REACHABILITY");
}
