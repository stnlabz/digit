# Digit Self-Diagnostics (DSD) 1.0.0

**Status: source candidate — Linux build, qualification and runtime acceptance pending.**

DSD is a stateless, read-only module exposing `dsd.diagnose`. It accepts a
bounded array of verified stage observations and counts of retrieved/selected
evidence and outbound-candidate status. It returns **OBSERVED**, **INFERRED**
or **UNKNOWN**, including the affected processing boundary and a bounded
explanation. It never writes runtime state, repairs code, or authorizes deployment.

DSD does **not** yet capture live diagnostic events or implement
`diagnose my last request`. No authenticated session-scoped correlation
service is currently exposed by this module; implementing that command without
one would risk disclosing another user's previous request. A separate,
human-approved integration design is required before enabling conversational
access. No Core or external ABI edits are proposed here.

Build using `cd modules/dsd && make clean && make && make test`, with
the sibling external ABI at `../../../ABI/includes/module.h`.

The caller must provide telemetry it has already verified. DSD will not
represent caller-supplied observations as independently collected facts.
Unknown data remains unknown.

## Example request

```c
digit_dsd_request_t request = {0};
request.retrieved = 1;
request.selected = 1;
request.candidate_constructed = 0;
/* Expected: INFERRED / NO_ANSWER_CANDIDATE. */
```

No production Corpus, Core, Interface, Dispatcher, or ABI changes.
