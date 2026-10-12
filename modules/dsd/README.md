# Digit Self-Diagnostics (DSD) 1.2.1

**Status: committed source candidate; local Linux build, qualification and live acceptance pending.**

DSD remains a deterministic, read-only module exposing the byte-compatible legacy `dsd.diagnose` and additive `dsd.diagnose_engineering`.
The engineering result adds affected module, observed symptom, source investigation path, and location evidence status. Only known module source files are mapped: these are **investigation targets**, never verified root causes or exact function/line claims. Unsupported or unobserved cases are explicitly unresolved. The legacy service keeps its original response size and includes a brief handoff in the report text, so already active Dispatcher binaries remain compatible. Its input consists of a bounded list of verified stage observations, plus
retrieved/selected evidence counts and candidate/validation flags. Its output
distinguishes reported processing boundaries from unverified root causes.

## Request-local live telemetry path

1. Response 1.9.7 provides the **additive** `response.answer_observed` service.
   It exposes evidence retrieval and selection counts, composition disposition,
   and Validator outcome codes. Existing `response.answer` output remains
   byte-compatible.
2. Dispatcher 1.3.10 provides the **additive**
   `dispatcher.handle_scoped_context_observed` service.
   It records Interpretation and Intent status, invokes observed Response,
   and builds a bounded DSD request. Existing Dispatcher services retain their
   original request and response schemas.
3. Interface carries only the immediately prior turn's bounded
   observations in the authenticated private session. Session revocation or
   expiry discards them; no cross-account or channel history is consulted.
4. A private `Digit, diagnose my last request.` routes the prior observations
   to `dsd.diagnose`. If the previous turn supplied no stage evidence, DSD
   explicitly reports that limitation.

This is **processing-outcome telemetry**, not a complete independent trace of
every module's internal execution. A Corpus record count comes from Response's
retrieval result; a Response composition failure is the observed response phase,
not an established defect in a specific source file. The root cause may remain
unknown.

No autonomous repair, deployment authority, Core modification, external ABI
modification, or production Corpus modification is included.

## Qualification order

From a development checkout with the sibling `ABI` available:

```bash
cd modules/response && make clean && make && make test
cd ../dsd && make clean && make && make test
cd ../dispatcher && make clean && make && make test
cd ../interface && make clean && make && make test
```

Only after all builds/tests succeed should the operator proceed to
independent runtime qualification and verify the qualified module versions.
Then exercise a two-turn conversation inside the **same authenticated private
session**; a new session intentionally cannot see the earlier request.

Recommended activation order: Response → DSD → Dispatcher → Interface.
During partial activation, additive-service fallback preserves ordinary
conversation but provides reduced or unavailable telemetry.

No fabricated diagnoses, no global last-request state, and no self-approval.
