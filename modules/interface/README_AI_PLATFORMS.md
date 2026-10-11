# AI Platform Interface Accounts — source candidate (2026-10-11)

**Not operationally qualified. Do not deploy as an accepted account-management release.**

## Implemented in this source slice

- Separate, operator-maintained credential file `/opt/digit/state/auth/ai_platform_accounts.tsv`, using the existing crypt-hash verification format.
- Independent capability registry `/opt/digit/state/auth/ai_platform_permissions.tsv` with exact lines `identity<TAB>active<TAB>dm<TAB>diagnostics<TAB>memory_write<TAB>learning<TAB>developer`. A sample identity is `ai.chatgpt`.
- The dedicated `POST /ai/session/login` path issues a normal Interface session token after checking both registries. Human login rejects the `ai.` account namespace.
- Authorized AI tokens use the existing `POST /ask` private-session state, preserving prior turns and DSD telemetry; all other protected administration and channel routes reject AI tokens.
- The existing `/ask` prohibition on shared-Corpus learning writes remains in force. AI memory-write phrases are denied unless `memory_write=1`.
- Permissions are evaluated on each protected request; a disabled account fails closed.

## Important limitations — not yet implemented

1. SA-controlled account creation, credential rotation, revocation and GUI management. Both files are still operator-managed; no AI account is created automatically.
2. The ChatGPT-facing runtime connector, credential exchange and direct ChatGPT-to-Digit round trip.
3. Complete per-capability enforcement for diagnostics, learning and developer authority. A flag in the registry **is not** proof that its operation is enabled or authorized. No privileged learning or developer action is wired through this slice.
4. Comprehensive route-level integration tests, module hotload qualification, and live acceptance. The parser has isolated negative-case tests only.

## Qualification gate

Do not treat a GitHub commit, a parser test, or Interface hotload as proof that ChatGPT can authenticate and talk to Digit.

The required acceptance test remains: SA provision `ai.chatgpt`, authenticate through a connected ChatGPT tool, send a normal `/ask` request and then a DSD follow-up using the same token, and verify correct authorization, denial and audit evidence.
