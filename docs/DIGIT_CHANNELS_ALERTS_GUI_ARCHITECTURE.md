# Digit Channels, Alerts, and Operator Console Architecture

Status: DEVELOPMENT BASELINE
Target: Digit 1.1

## 1. Purpose

Digit will support multiple persistent discussion channels, actively report operational faults to the human operator, and expose both capabilities through the Digit GUI.

The design preserves four separate concerns:

- Corpus = retained knowledge.
- Channels = discussion state and history.
- Audit = authoritative historical evidence.
- Alerts = current operator-visible operational conditions.

These stores will not be conflated.

## 2. Human Authority

Human authority remains controlling. Channels, alerts, modules, and GUI components do not acquire authority over Digit Core or the human operator.

A module failure will not terminate Digit Core. A module may become unavailable and Digit may enter a degraded capability state, but Core continues whenever Core itself remains operational.

## 3. Channels

Digit will support multiple named discussion channels.

Each channel has:

- immutable channel ID;
- human-readable name;
- creation timestamp;
- last activity timestamp;
- active/inactive state;
- ordered message history.

Each message has:

- immutable message ID;
- channel ID;
- timestamp;
- origin: human, Digit, system, or module;
- message body.

Conversation context is channel-scoped by default. Activity in one channel will not silently become conversational context in another channel.

Initial service surface:

- channel.create
- channel.list
- channel.get
- channel.message.append
- channel.message.list

A default `general` channel will exist when no explicit channel is supplied.

## 4. Alerts

Digit will provide a structured operator alert service.

Alert severity:

- INFO
- WARNING
- ERROR
- CRITICAL

Each alert has:

- immutable alert ID;
- timestamp;
- severity;
- source;
- summary;
- detail;
- operational state;
- acknowledged state;
- acknowledgment timestamp when applicable.

Initial service surface:

- alert.raise
- alert.list
- alert.get
- alert.acknowledge

Alerts are reports, not authority grants.

Modules may report faults through the alert service. Raising an alert does not permit a module to alter Core authority, Core doctrine, operational state, or another module.

## 5. Required Operational Reporting

Digit will raise an operator-visible alert when a material condition affects her own operation, including:

- module discovery rejection;
- module qualification failure;
- module activation failure;
- required service unavailable;
- degraded Core capability;
- failed learn operation;
- failed response operation when the failure is operational rather than an ordinary lack of evidence;
- Safe Mode entry;
- Company Preservation entry;
- other conditions requiring human awareness or intervention.

Where known, the alert will identify what failed, why it failed, what capability is affected, and whether Digit continues operating.

A module failure alone does not shut Digit down.

## 6. Audit Relationship

Operational alerts and channel activity will produce appropriate audit events.

Audit remains the authoritative historical evidence trail. Alerts remain the operator-facing active condition store. Acknowledging an alert does not delete or rewrite its audit history.

## 7. Interface API

The Interface module will expose channel and alert operations to approved clients.

The existing ask/learn behavior will be extended so an ask may carry a channel ID. Responses are appended to that channel's history.

The Interface module will return meaningful structured failure information rather than an unexplained HTTP 503 whenever Digit knows the underlying reason.

HTTP transport status and Digit operation result will agree. A successfully stored learn will not be reported to the client as a failed operation.

## 8. GUI Operator Console

The Digit GUI will become an operator console with three primary regions.

### Channel navigation

Displays available channels, unread activity, creation controls, and the currently selected channel.

### Discussion view

Displays ordered history for the selected channel and provides ask/learn interaction in that channel.

### Operational status and alerts

Displays:

- Core state: GREEN, DEGRADED, SAFE MODE, or COMPANY PRESERVATION;
- active module/service health when available;
- unread alert count;
- alert severity;
- alert source and summary;
- alert detail;
- acknowledgment control.

Critical operational information will not depend on the operator manually reading audit.log.

## 9. Failure Behavior

If a discussion-supporting module fails:

1. Digit Core continues.
2. The failed capability is marked unavailable.
3. Digit raises an alert identifying the failure and reason when known.
4. Interface returns a meaningful degraded/unavailable response.
5. GUI displays the condition to the operator.
6. Audit records the event.

If Core itself encounters a qualifying Core fault, existing Safe Mode and Company Preservation rules remain controlling.

## 10. Implementation Order

Phase 1: channel storage and channel services.

Phase 2: alert storage and alert services.

Phase 3: Core/module-manager operational alert hooks.

Phase 4: Interface channel and alert API.

Phase 5: GUI operator console update.

Phase 6: failure, isolation, persistence, restart, and negative validation tests.

## 11. Design Rule

Digit will remain small, deterministic, and easy to operate.

Channels provide discussion state. Corpus provides knowledge. Audit provides evidence. Alerts provide operator awareness. None substitutes for another.
