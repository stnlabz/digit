# Digit Memory 1.0.0

Private durable account memory module. Services: memory.put and memory.get.
Account IDs must be validated by an authenticated Interface or Slack session; callers must never derive identity from conversation text. The module itself validates account-ID syntax but cannot independently authenticate the caller. It stores fixed-size records in /opt/digit/state/memory, with owner-only directory and record permissions. A successful put is fsynced; get returns the most recently recorded item. Records persist through logout, restart, and supported app switches when using the same Digit server and authenticated account.

Current v1.0.0 is deliberately one-record retrieval, not full episodic retrieval, memory editing, multi-platform database synchronization or natural learning. Do not deploy to production without Linux test qualification and privacy/data-retention review.

## Qualification and deployment

From `~/digit/modules/memory`, run `make clean && make && make test`; after review and a configured `digit` service account, install with `sudo make install` and ensure the module is loaded. Build and deploy updated Slack 1.0.9 and Interface 1.7.9. In an authenticated DM or native `/ask`, `remember ...` or a statement beginning `I am ` saves a record, and `What do you remember about me?` retrieves the latest saved entry. A single latest record is currently returned. Security qualification must assess memory-service call authorization before production deployment; the service ABI trusts the calling authenticated interface to supply the verified actor.

## Conversational presentation helper

The shared `includes/memory_response.h` provides bounded conversational presentation of stored text for both Interface 1.7.10 and Slack 1.0.10. It changes the response phrasing, not memory storage or authentication. The source is retained verbatim; Linux tests include speaker-pronoun and greeting regression checks.
