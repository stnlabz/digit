# Digit Slack Bridge — Socket Mode

This integration runs independently of Digit Core. It uses the existing HTTPS
Interface session and private-request endpoints, not direct Dispatcher calls.

**Status: implementation candidate.** No Slack workspace deployment, server
acceptance, or live identity verification is claimed by this source commit.

## Slack app setup

Enable Socket Mode. Grant the app-level token `connections:write`. Grant the
bot `app_mentions:read` and `chat:write`; subscribe to `app_mention`.
For DMs, add `im:history` and the `message.im` event. Reinstall after
scope changes. Never commit full `xapp-` or `xoxb-` tokens.

## Install

Use a separate unprivileged Linux service user and a Python virtual environment:

```sh
cd integrations/slack
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
.venv/bin/python -m unittest -v test_bridge.py
```

Export `SLACK_APP_TOKEN`, `SLACK_BOT_TOKEN`,
`DIGIT_INTERFACE_URL=https://digit.stn-labz.com:8081`, and
`DIGIT_SLACK_IDENTITIES=/path/to/private/identities.json` through an
operator-managed service secret store, not a committed file. Then run
`.venv/bin/python bridge.py`.

The identities JSON maps exact Slack user IDs to their individually authorized
Digit credentials:

```json
{"U_REPLACE_WITH_REAL_ID": {"username": "digit-user", "password": "REPLACE_WITH_SECRET"}}
```

Keep this file owned by the bridge process's operating-system user, mode 0600.
This is a temporary deployment enrollment mechanism; avoid storing credentials
long term when a separately qualified account-linking mechanism becomes available.
**Never map Slack users to a shared SA account.**

## Security and behavioral contract

- A Slack user must be explicitly enrolled; no Slack role grants Digit privileges.
- Each request logs in using that user's own Digit credentials, asks via `/ask`
  and logs out. Core and authorization policy are unchanged.
- The bot responds only to explicit mentions and direct messages, not every
  channel message, and ignores bot-origin messages.
- It does **not** access Digit projects, channels, grants or administrative APIs.
- Slack workspace/channel permission is a separate prerequisite; Slack content
  delivered to the bridge is still subject to Digit's own policy.
- `/ask` is Digit-private. Do **not** use this MVP for private or sensitive data
  in public Slack channels: replies are visible to other channel members.
- Run under trusted HTTPS and validate the server certificate; do not disable
  TLS verification.
- The one-account-per-Slack-actor model prevents a shared Digit conversation
  identity but is not full scoped Slack-channel-history support.
- Test deduplication/retries, concurrency, session cleanup, credential rotation,
  and end-to-end authorization before operational promotion.
