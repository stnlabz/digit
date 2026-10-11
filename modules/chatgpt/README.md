# Digit ChatGPT Integration — 1.0.0 candidate

ISO C module hosted in the existing Digit process. This is an integration
foundation, not an operational ChatGPT endpoint.

- `chatgpt.capabilities` reports that Interface integration and conversations
  are **disabled** pending qualification of authenticated, non-mutating access.
- `chatgpt.test_case` always refuses execution; it **never** dispatches an
  arbitrary request into operational services.
- Learning requires an authenticated principal with explicit write authority.
  ChatGPT test access confers **no** learning, Corpus, Memory or other mutation
  permission. Authentication alone is not sufficient to authorize learning.
- No listener, additional process, Core edit, external ABI edit or deployment.
- The separately required Interface binding, service-level identity/scope
  enforcement and runtime connector remain **unimplemented**.

Build and test from this directory with `make all && make test`.
Unit/qualification tests demonstrate denial behavior, not live interaction.
