# Patch061 — Preserved legacy runtime-harness evidence

This record preserves unique harness findings from a private, hash-verified
legacy `CURRENT_STATE.md` snapshot (SHA-256
`c261642c8558e31caaa2fe4e4be074c12884e8b186591db11702da1b0826267f`). The
source snapshot is retained outside the repository. Later Patch071/Patch072
work superseded Patch061 as the project resume target; it does not erase these
historical results. No local machine paths, endpoint values or device
identifiers are retained here.

## Runtime evidence classification

- Patch061's recorded firmware SHA-256 was
  `d499abb34aa7f5eccbea590e3780190c3b1878f2d34b1ee7de1ee60dd3971367`.
  App-only flash and HTTP startup were recorded PASS, but runtime diagnostics
  were **INCONCLUSIVE** and root cause **NOT_PROVEN**.
- `ESP_ERR_HTTP_CONNECT` (`0x7002`) was observed in the journal. Homey recovery
  was not observed; a fresh snapshot and awning runtime read were not reached.
  These observations do not prove the underlying transport cause.

## Artifact-local harness findings

- The harness work was artifact-local and did not change firmware/source.
  A direct `valid:false` no-read regression was reported PASS: invalid
  diagnostics did not access `raw_transport` or `counter_deltas`, and stale
  sample-A values did not appear. A separate continuation manifest was
  reported as 29/29, but that aggregate does not override the specific
  unresolved and failing assertions below.
- Malformed JSON was safely caught as `JSONDecodeError`; however, the
  artifact-local validator expected an exact `ValueError` classification.
  This is an assertion mismatch, so the malformed-JSON validator was **not
  fully PASS**.
- Type validation exposed a substantive defect: a wrong-typed `attempt` value
  was accepted and projected. This check **FAIL**ed. Remaining harness checks
  were stopped at that first substantive failure and are **NOT_RUN**.
- The harness candidate and the later frozen harness were recorded with the
  same SHA-256:
  `1c369a09d705379a6e001cea6056fc4aebe01bb04427c3e7b270b928bc4badd1`.
  Evidence provenance is recorded in the preserved private audit; its local
  artifact paths are intentionally omitted from this public history entry.

## Scope and limits

No Patch061 runtime recapture was authorized. These harness results are not a
firmware regression result and do not change the separate runtime classification
above. Patch071/Patch072 superseded the proposed Patch061 follow-up; this record
does not reopen it.
