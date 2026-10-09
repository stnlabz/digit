# Digit

**STN-LABZ** · Modular, platform-independent autonomous-agent development

**Engineering principle:** Small, deterministic, and easy to use. **Determinism ≠ Probably.**

Digit follows **Core → Modules → Mission**. The common Core binds through its platform interface, with target-specific behavior outside shared Core semantics. Architecture, authorization and deployment remain under human control.

## General Orders

1. Remain at the assigned mission.
2. Follow established policies and authorized instructions.
3. Report anything outside delegated authority to the appropriate next level of authority.

Modules and mission packages cannot override these orders.

## Current development status

**Evidence cutoff: operator-reported 2026-10-09 UTC.** The last explicitly verified production Interface is **1.5.10**, with `MODULE QUALIFICATION_GREEN`, HTTPS listener activation and `MODULE HOTLOAD_ACTIVE` at **15:25:50 UTC**.

| Milestone | Qualification | Runtime evidence |
| --- | --- | --- |
| 1.5.4 | 14/14 dedicated SA checks; historical suites passed | GREEN / ACTIVE 05:15:52 UTC |
| 1.5.5 | TLS remote access / HTTPS; historical tests passed | Initial startup failed without cert/key; subsequently HTTPS verified locally and from Windows |
| 1.5.6 | 14/14 dashboard checks; 34/34 historical suites | Operator reported installed; runtime dashboard displayed SA aggregates |
| 1.5.7 | Version increment for refreshed dashboard release | Tested/installed status not separately established in this summary |
| 1.5.8 | 14/14 channel-name boundary checks; historical suites passed | GREEN / ACTIVE 14:44:07 UTC |
| 1.5.9 | 18/18 organization SA policy checks; 36/36 historical suites | GREEN / ACTIVE 15:17:54 UTC |
| **1.5.10** | **16/16 project-list checks; 37/37 historical suites** | **GREEN / ACTIVE 15:25:50 UTC** |

The historical Interface 1.3.x–1.4.x milestone record is retained in [Development Roadmap](docs/ROADMAP.md). Unit tests, module qualification, activation, and end-to-end authorization are **distinct** evidence gates.

## Interface and Windows GUI

- HTTPS endpoint: **`https://digit.stn-labz.com:8081`**. Interface listens on the IPv4 wildcard address and requires TLS. Windows HTTPS validation was confirmed against the operator-installed temporary self-signed certificate; replace it with an appropriately trusted certificate for sustained operations.
- `poemei` authenticated through Digit GUI. The Core-controlled STN-LABZ SA read-access check was observed successful.
- The read-only `GET /admin/dashboard` reports Core channel/alert totals; the GUI displayed four channels and two alerts in operator testing. Dashboard counts do **not** confer channel ACL access.
- GUI and Interface 1.5.9 introduced SA project creation and Security binding controls. Interface 1.5.10 and GUI 1.5.10 add organization-scoped project listing subject to current SA assignment and project membership.
- `POST /channels` in 1.5.8 allows an authorized SA to request Core channel creation; this does **not** automatically grant visibility or membership.

### Project storage and explicit authority

Protected project root: `/opt/digit/state/projects/<organization>/<project>/`. Project provisioning records include `project.tsv`, `security.tsv` and `READY`; restricted Security-channel binding uses `security_channel.id`. The channel-access registry is **`/opt/digit/state/auth/channel_grants.tsv`**, not `channel_acl.tsv`. Missing, invalid or unauthorized grants fail closed.

Organization and project scopes are separate. A valid SA assignment to **stn-labz** does not authorize **team-chaos**. The Founder may hold both assignments independently, subject to the controlled roster; that second assignment has not been evidenced as provisioned. Project and channel access also require their corresponding membership/grants.

**Operator-directed target layout:**
- **STN-Labz:** `learn`, permanent SA `Alerts`, permanent SA `Security`.
- **Team ChAoS:** permanent SA `Security`.
- The existing legacy `general`, `learn`, `alerts`, and `Team ChAoS` channel records are not automatically migrated into those scopes. Preserve their message history.
- The right-side GUI panel is intended to become **channel Presence**. Operational events belong in the permanent organization-scoped Alerts channel; live presence must be tracked separately.

**Not yet complete:** GUI-managed scoped channel grants, permanent SA channel provisioning and lifecycle enforcement, organization-isolated presence, legacy channel migration, and end-to-end multi-organization access qualification. Do not equate the 1.5.9 policy test or 1.5.10 listing test with these features being live.

## Build and qualification

From `modules/interface`, with the sibling external ABI available:

```sh
make clean && make && make test
```

For the native Windows GUI, use its repository's `build.cmd` in an MSVC environment. Do not publish credentials, account hashes, private keys or authorization registries.

Interface 1.5.10: **16 dedicated checks and 37 historical suites passed**, with `MODULE HOTLOAD_ACTIVE` observed. Any new source change requires compilation, positive/negative tests, complete regression qualification, and operator review before activation.

## Next development phase: 1.6

1. Finish organizational isolation and explicit GUI-managed channel/project grants.
2. Provision and protect per-organization permanent Security/Alerts channels (no automatic cross-org inheritance).
3. Build authenticated per-channel Presence with explicit join, leave, expiry and visibility rules; maintain alert event records separately.
4. Demonstrate negative cross-organization access cases and migration safety before treating the new organization model as operationally qualified.
5. Then proceed to mission-oriented bounded work, escalation and structured audit under the General Orders.

These are **planned objectives**, not claims of implementation or deployment approval.

## Engineering boundaries

Core, external ABI and common autonomous-agent architecture changes require explicit human authorization. Preserve deterministic behavior, annotated source changes, negative tests, historical regression, and the Module Creation Request process when applicable.

See [Development Roadmap](docs/ROADMAP.md) for detailed evidence, development phases and outstanding gates.

*Engineering systems worthy of trust when trust matters most.*
