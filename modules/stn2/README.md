# STN-2 Intelligence Module — initial collection capability

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
HTTP failures and empty responses are marked unavailable.

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
This first milestone performs bounded retrieval and reports source health; it
does **not** yet parse CVEs, correlate records, store a durable intel history,
generate the weekly briefing, or verify advisory findings. It intentionally
does not claim those capabilities or fabricate intelligence.

Deployment acceptance requires valid feeds, source-content analysis,
operator authorization testing, network failure recovery, evidence retention,
and an actual `@Digit gen intel` Slack report.
