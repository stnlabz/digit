# STN-2 Intelligence Module — version 1.0.5 (Sentinel schema-aware analysis)

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
This revision performs bounded retrieval, captures and validates the small Sentinel `/intel` JSON summary, reports its historical type counts, 24-hour activity and pattern indicators, and extracts unverified CVE references from the byte streams. It does **not** yet verify advisory findings, correlate Rictus against individual threat records, persist investigations, autonomously watch cases or generate the weekly briefing. It intentionally
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
