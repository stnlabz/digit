# STN-2 Intelligence Module — version 1.1.1 (intelligence-first reporting)

Native ISO C11 Digit module. It registers `stn2.generate_intel` on the existing
module ABI service bus. The Slack module invokes this service for
`@Digit gen intel` or `@Digit generate feed` when issued in the configured
private intelligence channel.

## Configuration

Module: `/opt/digit/modules/stn2/`

**Private configuration:** `/opt/digit/auth/stn2/stn2.conf`, owned by root
or the Digit runtime account, mode 0600. Copy `stn2.conf.example` and set
`enabled=true` only after review. Missing/disabled/unsafe configs fail closed.

`rictus_log=/var/log/rictus/rictus.log`: existing text logs; the module
summarizes recent records and explicit WARN/ERROR/ALERT keywords, without
assuming each is a verified threat.

`api_base=https://api.stn-labz.com`: fetches
`/threats`, `/events`, `/intel`, `/patterns`, `/rss` read-only.
HTTPS retrieval uses a streaming byte-count callback with an 8 MiB per-response safety limit, rather than buffering a fixed 32 KiB payload. Reports distinguish RETRIEVED, SIZE_LIMIT, HTTP_ERROR, NETWORK_ERROR, EMPTY_RESPONSE, and INVALID_URL, including observed byte count and HTTP status. HTTP 200 alone does not establish content validity or verified intelligence.

`advisory_list=/opt/digit/auth/stn2/advisories.list`: separate private
(mode 0600) UTF-8 file with one operator-approved absolute HTTPS advisory URL
per line (at most 32). CMS, MVC, Apache2, LiteSpeed, NGINX and IIS feeds can be
configured here. It does not automatically discover or trust arbitrary sites.

Slack `/opt/digit/auth/slack/slack.conf` also needs
`intel_channel_id=<the private Intel-Briefs channel ID>`.

## Qualification and limitation

`make test && make all` in `modules/stn2` requires the separate
STN-LABZ ABI and native libcurl development library.

**SOURCE CANDIDATE — NOT A QUALIFIED INTELLIGENCE ANALYST.**
This revision performs bounded retrieval, captures and validates the small Sentinel `/intel` JSON summary, reports its historical type counts, 24-hour activity and pattern indicators, and extracts unverified CVE references from the byte streams. It does **not** yet verify advisory findings, correlate Rictus against individual threat records, persist investigation cases, autonomously watch cases or generate the weekly briefing. It intentionally
does not claim those capabilities or fabricate intelligence.

Deployment acceptance requires valid feeds, source-content analysis,
operator authorization testing, network failure recovery, evidence retention,
and an actual `@Digit gen intel` Slack report.

## Revision 1.0.1

Corrected false UNAVAILABLE results for large Threat API responses (such as ~613,400-byte /threats and /events feeds). Incoming bytes are counted incrementally, with a bounded 8 MiB ceiling and explicit failure classification. No claim of JSON/RSS interpretation or security evidence verification is made. Offline regression tests include >600 KiB receipt and over-limit rejection. Qualification and deployment must still be verified on the running Linux host.

## Revision 1.0.2

Fixes the streaming test assertion by capturing the remaining capacity before the callback mutates the received-byte counter. Separates misleadingly indented conditional statements in the module source. Module descriptor bumped to 1.0.2. Native build and runtime qualification still require verification.

## Revision 1.0.3

The bounded streaming collector now detects syntactically recognizable CVE identifiers across transfer chunk boundaries in each retrieved Threat API feed and reports the number of textual mentions plus up to eight distinct examples per source. Repeated identifiers within a feed are deduplicated for display. These are **unverified text references**, not security findings, and not yet evidence of affected products or exploitation. No JSON schema interpretation, historical correlation, severity determination, affected-product analysis, durable intelligence record or completed weekly brief is claimed. Tests cover split-chunk extraction, duplicates, and prior transfer limits.

## Revision 1.0.4

Corrected an offline test that passed a multi-megabyte size with only an 8 KiB source buffer. Since v1.0.3 actually examines streamed bytes for CVE identifiers, this caused an out-of-bounds read and segmentation fault. The test now iterates over actual 8 KiB chunks. Module descriptor revision incremented to 1.0.4; live qualification remains pending.

## Revision 1.0.5

Implements initial evidence-based interpretation of the provided Sentinel /intel schema (stats.total_threats, stats.by_type, analysis.total_24h, analysis.trending and patterns). The report differentiates historical totals from 24-hour values and explicitly states that request-pattern indicators are not proof of compromise. Parses only bounded /intel JSON content; raw /threats and /events remain byte-counted, not normalized or deduplicated. The advisory feed is not yet enabled by default. Native json-c development headers and library are required. New offline tests validate an /intel example and reject missing required fields.

**Not implemented:** durable evidence/case management, investigation decisions, timed watchers, weekly brief synthesis, or full CVE/vendor advisory verification. These remain required mission milestones; do not describe this version as autonomous investigation completion.

## Revision 1.0.6

Introduces cross-run **aggregate baseline tracking** for the Sentinel `/intel` total. After a valid authenticated HTTPS response and parsed schema, the module writes a private `/opt/digit/auth/stn2/intel-state.json` snapshot with 0600 permissions using a same-directory temporary file and atomic rename. On later collections it reports the change in `stats.total_threats`; a decrease is explicitly presented as a possible reset or retention change, not negative incident activity. Corrupt, inaccessible, or unsafe prior state files prevent baseline replacement and produce a visible diagnostic. All reads of external security sources remain read-only.

The Digit runtime user must have write access to `/opt/digit/auth/stn2/`. This is a **historical aggregate delta**, not proof of new distinct threat IDs, a durable incident evidence archive, correlations, completed investigations, or timed autonomous watches. Concurrent invocations are not yet serialized; aggregate state is only a candidate for later case tracking, not a reliable concurrent journal. Offline tests verify first baseline and second-run delta.

## Revision 1.0.7

On successful `/threats` retrieval, capture the response within the existing 8 MiB limit, validate the `threats` array, and compare stable `id` fields against a prior private snapshot at `/opt/digit/auth/stn2/threat-ids.json` (0600). The first collection establishes a baseline; subsequent reports show how many IDs are absent from the prior snapshot and list up to four new record IDs with reported type and timestamp. Duplicate IDs within a single collection are not double-counted. `/events` is *not* treated as an independent corroborating record set.

Scope and limits: maximum 4,096 threat array entries per snapshot; a larger or malformed feed fails closed for record comparison and leaves the previous ID baseline unchanged. State updates use same-directory temporary files and atomic rename. A separate aggregate baseline remains in `intel-state.json`. These IDs are snapshot comparisons, not a durable evidentiary journal or automated investigations. Full Rictus correlation, case management, continuous autonomous watches, investigative outcomes, and non-blocking Slack progress acknowledgements remain **unimplemented**. Concurrent snapshot writers are not serialized. Testing on the actual host is required before qualification.

## Revision 1.0.8

For the first four threat IDs absent from the prior snapshot, compares each record's top-level `ip` value with whole IP-like tokens in the most recent 120 readable Rictus log records. The report explicitly labels an exact text match as a candidate correlation, **not proof of a shared incident, attacker identity, successful compromise, or an independent confirmed event**. It distinguishes no match from unavailable log collection. Prefix/suffix digit and period boundaries avoid basic substring collisions. The trusted source IP role remains subject to the Threat API schema; `details.ip_address` is not conflated with top-level `ip`. New offline tests exercise exact matching, nonmatching substrings, and a second-run cross-source report.

This revision performs no active probes and does not mutate Rictus, Sentinel or Digit Core. This is limited evidence correlation, **not** a durable investigative case system, automated case watches, full record timelines, source-concordance verification, or non-blocking Slack job control. Concurrent snapshots are still not serialized. Linux build and live verification required.

## Revision 1.0.9

STN-2 now compares **all distinct valid threat IDs in each retrieved /threats snapshot**, not only newly appearing IDs, against whole IP tokens in the latest 120 Rictus log records. It reports the number of records with usable IPs checked, the number with matching Rictus tokens, and up to three illustrative candidate ID/IP matches even when zero IDs are new. This is a read-only historical correlation pass, distinct from snapshot change detection. A missing Rictus source is explicitly marked incomplete; missing IP fields are not counted as matches. Repeated threat IDs are skipped. Matching text alone is not incident confirmation, attribution, or evidence of exploitation.

This is **not** yet persistent case management, automatically scheduled watching, deep investigation, or complete source-evidence retention. Repeated log rescans per ID have a performance cost; benchmarking and concurrent snapshot serialization are outstanding before production-scale qualification. Offline tests include second and third snapshots with no new IDs to verify historical correlations persist.

## Revision 1.0.10

Historical correlation now reads and validates the Rictus log **once per STN-2 threat-record comparison**, retaining only the most recent 120 complete log lines in a bounded in-memory window. Every deduplicated Sentinel record is matched against that same snapshot, avoiding the previous per-record repeated file scans. Existing evidence caveats and per-source permissions remain unchanged. Unavailable Rictus input is explicitly reported as an incomplete correlation rather than as zero evidence. Additional regression checks exercise loaded-window matches and nonmatches. This change does not introduce durable investigations, watch scheduling, active probes or Slack background dispatch; these capabilities remain outstanding. Live build, performance and end-to-end qualification are required.

## Version 1.1.0 — Initial structured investigation assessments

STN-2 correlates the historical, deduplicated Sentinel threat records against a single bounded window of recent Rictus observations, using two independent textual candidates: the top-level `ip` token and `details.request_url` literal path. It reports checked and matching record counts separately, up to three IP examples and two path examples, and an explicit outcome: candidate overlap, no overlap in the bounded window, or insufficient evidence when Rictus is inaccessible. These are *candidate investigative assessments*, not verified incidents or successful exploitation. Request paths may contain query strings and are compared literally, not normalized. Timestamp chronology, domain identity, IP role attribution, provenance validation, evidence retention, durable case records, scheduled watches, and asynchronous Slack progress are **not implemented** in this release. Testing and operational qualification remain necessary.

## Revision 1.1.1 — Intelligence brief presentation

The Slack-facing `@Digit gen intel` report now prioritizes observed Sentinel categories, recent activity, tracked/new IDs, Rictus findings, candidate overlaps, and evidence-limited assessment. Successful transport byte counts, HTTP codes, per-feed status banners, repeated zero-CVE lines and generic collector notes are suppressed from the intelligence brief. Failed collection remains visible as a coverage gap; CVEs appear only when actual textual references exist. No source collection, analysis, access-control, or Core behavior changed. Tests are updated for the new report language. Remaining absence of case workflows is unchanged.
