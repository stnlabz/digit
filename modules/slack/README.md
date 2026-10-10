# Digit Slack Module (ISO C11)

Native Digit module. Uses the existing **Core module ABI service bus** and
the established Interpretation → Intent → Dispatcher → Response/Validator
path. Does not change Core or ship a separate bot/service.

## Secure configuration

The sole active configuration path is:

`/opt/digit/auth/slack/slack.conf`

Use `slack.conf.example` as a template; no credentials are committed. The
configuration must be an ordinary file owned by root or the running Digit
user and have mode **0600**. The module fails closed when missing, disabled,
insecure, or malformed.

```ini
enabled=true
workspace_id=T_REPLACE
app_token=xapp-REPLACE
bot_token=xoxb-REPLACE
```

These placeholders are not usable credentials. Never post tokens to the chat,
log, source control, or a shared Slack channel. Rotate credentials if exposed.

## Slack app configuration

Enable **Socket Mode** and give the app-level token `connections:write`.
Grant the bot `app_mentions:read` and `chat:write` and subscribe to
`app_mention`. To enable direct messages, grant `im:history` and subscribe
to `message.im`. Reinstall the app after changing scopes, and invite Digit
to channels where mentions are required.

## Build and qualification

Requires ISO C11 compiler, libcurl **with WebSocket support** (7.86.0 or
newer), json-c, pthread, and the separate STN-LABZ ABI repository.

```sh
cd modules/slack
make test
sudo make install
sudoedit /opt/digit/auth/slack/slack.conf
sudo chmod 0600 /opt/digit/auth/slack/slack.conf
```

Install only creates a disabled template if the file does not exist. Digit's
own module manager must qualify and activate the module; installation does
not establish that the module is active. The worker establishes an outbound
WebSocket with `apps.connections.open`, acknowledges Slack envelopes, calls
Digit's scoped Dispatcher through the host ABI and posts the answer with
`chat.postMessage` to the source channel/thread.

## Conversation and authority

Anyone in the configured workspace who can mention the installed Slack bot
may ask questions. Users need not separately register a Digit account for
this **open conversational endpoint**. The request actor is marked
`slack:<Slack-user-id>`, private-scoped; Slack does not claim a Digit
organization, project, channel membership, SA qualification or admin
authority. This module does not expose administrative service calls.

The Slack transport and event IDs are untrusted. Tokens are never included
in state reports. No claim of live workspace validation or successful
end-to-end acceptance is implied by the initial source commit.

**Operational promotion requires:** compiler and module qualification GREEN,
Socket Mode connected, real `@Digit` replies observed, DMs verified (if
enabled), workspace isolation tests, repeated-event deduplication, reconnect
testing, and authorization negative tests. In particular, confirm that the
scoped Dispatcher denies protected organization data to Slack actors.
