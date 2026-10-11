# Digit ChatGPT Integration — 1.0.3 source candidate

ISO C module hosted in Digit's existing process. **Not yet an operational
ChatGPT OAuth connector.** No live OAuth endpoint or MCP client connection is
claimed by this source candidate.

## ChatGPT-local authentication primitives

- `chatgpt_auth.c` recognizes the existing **Bearer + 64 lowercase hex**
  token header syntax. It does **not** authenticate the token, establish an
  identity, authorize a request, or replace server-side credential checks.
- `digit_chatgpt_pkce_s256` implements RFC 7636 S256 challenge verification.
  It validates input character sets/length and compares the challenge in
  constant time. The module does not yet issue authorization codes or tokens.
- These primitives are built into `chatgpt.so` and covered by
  `tests/test_chatgpt_auth.c`. No additional listeners or processes are added.
- `chatgpt.capabilities` continues to report that Interface binding and live
  conversations are disabled. `chatgpt.test_case` still denies execution.
  The existing `chatgpt.mcp` service only supports limited protocol probes.

## Remaining integration boundary

For an actual ChatGPT MCP OAuth connection, a qualified transport must expose
OAuth authorization-server and protected-resource metadata, an SA-controlled
consent/authorization-code endpoint, PKCE-protected code redemption and
appropriate access-token validation, while retaining the existing identity
and capability controls and private-session DSD continuity.

**These endpoints do not exist in this module yet.** Simply validating a PKCE
challenge or bearer string does not authenticate a ChatGPT user. In particular,
the existing `POST /mcp` HTTP transport currently belongs to Interface and
does not invoke these helpers. No Core or Interface files were modified in
this change, and no authorization boundary has been bypassed.

Build and unit-test (Codex must run and report actual results):

```sh
make -C modules/chatgpt all
make -C modules/chatgpt test
```

Unit tests are not runtime qualification. Prior to claiming connectivity,
Codex must verify negative authorization tests and a real ChatGPT-to-Digit
initialize, tools/list, and two-turn digit_ask round trip. Learning,
Memory/Corpus mutation, Developer and SA authority are never conferred by
authentication alone.
