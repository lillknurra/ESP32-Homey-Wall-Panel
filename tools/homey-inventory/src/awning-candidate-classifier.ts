import {
  AWNING_ACTIONS,
  AWNING_ROLES,
  type AwningAction,
  type AwningCandidateClassification,
  type AwningRole,
  type CandidateObservation,
  type ClassifiedActionEvidence,
} from "./awning-model.js";

function candidateClassification(kind: CandidateObservation["source_kind"]): AwningCandidateClassification {
  if (kind === "device_capability") return "CANDIDATE_DEVICE_CAPABILITY";
  if (kind === "flow") return "CANDIDATE_FLOW";
  return "CANDIDATE_ADVANCED_FLOW";
}

function classifyOne(
  observations: readonly CandidateObservation[],
  collectionComplete: boolean,
): ClassifiedActionEvidence {
  if (observations.length === 0) {
    return {
      classification: collectionComplete ? "NO_CANDIDATE" : "INSUFFICIENT_EVIDENCE",
      candidate_source_aliases: [],
      blocking_reasons: collectionComplete ? [] : ["COLLECTION_INCOMPLETE"],
    };
  }

  const sourceAliases = [...new Set(observations.map((item) => item.source_alias))].sort();
  const blockers = [...new Set(observations.flatMap((item) => item.blocking_reasons))].sort();
  const sideEffectAmbiguous = observations.some((item) => item.side_effect_device_count > 1);
  if (sourceAliases.length > 1 || sideEffectAmbiguous) {
    return {
      classification: "AMBIGUOUS",
      candidate_source_aliases: sourceAliases,
      blocking_reasons: [
        ...(sideEffectAmbiguous ? ["MULTI_DEVICE_OR_MULTI_ACTION_SIDE_EFFECT"] : []),
        ...blockers,
      ].sort(),
    };
  }

  if (blockers.length > 0) {
    return {
      classification: "INSUFFICIENT_EVIDENCE",
      candidate_source_aliases: sourceAliases,
      blocking_reasons: blockers,
    };
  }

  const kinds = new Set(observations.map((item) => item.source_kind));
  const signatures = new Set(observations.map((item) => item.structural_signature));
  if (kinds.size !== 1 || signatures.size !== 1 || observations.length !== 1) {
    return {
      classification: "AMBIGUOUS",
      candidate_source_aliases: sourceAliases,
      blocking_reasons: ["COMPETING_STRUCTURAL_CANDIDATES"],
    };
  }

  return {
    classification: candidateClassification(observations[0]!.source_kind),
    candidate_source_aliases: sourceAliases,
    blocking_reasons: [],
  };
}

export function classifyAwningCandidates(
  observations: readonly CandidateObservation[],
  collectionComplete: boolean,
): Record<AwningRole, Record<AwningAction, ClassifiedActionEvidence>> {
  const result = {} as Record<AwningRole, Record<AwningAction, ClassifiedActionEvidence>>;
  for (const role of AWNING_ROLES) {
    result[role] = {} as Record<AwningAction, ClassifiedActionEvidence>;
    for (const action of AWNING_ACTIONS) {
      result[role][action] = classifyOne(
        observations.filter((item) => item.role === role && item.action === action),
        collectionComplete,
      );
    }
  }
  return result;
}

export type AwningBackendCandidateContract =
  | "COMMON_BACKEND_CANDIDATE_CONTRACT"
  | "INDIVIDUAL_OR_MIXED_CANDIDATE_CONTRACT"
  | "CONTRACT_UNDETERMINED";

export function classifyBackendCandidateContract(
  observations: readonly CandidateObservation[],
  classified: Record<AwningRole, Record<AwningAction, ClassifiedActionEvidence>>,
): AwningBackendCandidateContract {
  for (const role of AWNING_ROLES) {
    for (const action of AWNING_ACTIONS) {
      const classification = classified[role][action].classification;
      if (classification !== "CANDIDATE_DEVICE_CAPABILITY"
          && classification !== "CANDIDATE_FLOW"
          && classification !== "CANDIDATE_ADVANCED_FLOW") {
        return "CONTRACT_UNDETERMINED";
      }
    }
  }
  for (const action of AWNING_ACTIONS) {
    const signatures = new Set<string>();
    const kinds = new Set<string>();
    for (const role of AWNING_ROLES) {
      const item = observations.find((candidate) => candidate.role === role && candidate.action === action
        && candidate.blocking_reasons.length === 0 && candidate.side_effect_device_count <= 1);
      if (!item) return "CONTRACT_UNDETERMINED";
      signatures.add(item.structural_signature);
      kinds.add(item.source_kind);
    }
    if (signatures.size !== 1 || kinds.size !== 1) return "INDIVIDUAL_OR_MIXED_CANDIDATE_CONTRACT";
  }
  return "COMMON_BACKEND_CANDIDATE_CONTRACT";
}
