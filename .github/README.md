# Digit

**STN-LABZ Engineering Intelligence**

> **Engineering systems worthy of trust when trust matters most.**

![Language](https://img.shields.io/badge/Language-ISO%20C-blue)
![Status](https://img.shields.io/badge/Status-Active%20Development-orange)
![Architecture](https://img.shields.io/badge/Architecture-Core%20%2B%20Modules-blueviolet)
![Core
Qualification](https://img.shields.io/badge/Core%20Qualification-20%20Tests-yellow)
![Module
Qualification](https://img.shields.io/badge/Module%20Qualification-10%2B%20Tests-yellow)
![Internet](https://img.shields.io/badge/Public%20Internet-Isolated-critical)

Digit is a private autonomous engineering agent developed by
**STN-LABZ** to assist authorized engineers in the execution of STN-LABZ
engineering missions.

Digit is not a public AI service, general-purpose chatbot, or
conversational companion.

She is an engineering system.

Her purpose is to understand the STN-LABZ engineering environment,
operate within an assigned mission, interpret and enforce approved
engineering policy and doctrine, provide essential core engineering
capabilities, extend her mission capabilities through qualified modules,
and preserve the integrity of the STN-LABZ ecosystem.

------------------------------------------------------------------------

## Status

**Development**

Digit is undergoing a ground-up implementation in **ISO C**.

Earlier versions of Digit were experimental implementations with a
substantially different mission and architecture. Historical source may
be evaluated for useful engineering concepts, but current architecture,
policy, and requirements take precedence over previous implementations.

------------------------------------------------------------------------

## Mission

Digit exists to assist STN-LABZ engineers with authorized engineering
work.

Her operational behavior is mission-driven, concise, professional,
evidence-based, and deliberately non-conversational.

A fundamental interaction with Digit is intentionally simple:

``` text
Good morning.

What is today's mission?
```

Digit does not invent her own mission.

Human authority assigns the mission. Digit executes within its
established boundaries.

------------------------------------------------------------------------

## Architecture

Digit follows the STN-LABZ nucleus-outward engineering model:

``` text
Digit
│
├── Core
│   ├── Identity
│   ├── Mission
│   ├── Doctrine
│   ├── General Orders
│   ├── Policy Interpretation
│   ├── JSON Interpretation
│   ├── Markdown Interpretation
│   ├── State Management
│   ├── Safe Mode
│   ├── Qualification
│   ├── Module Governance
│   └── Audit / Reporting
│
└── Modules
    └── Mission-Specific Engineering Capabilities
```

The defining architectural rule is:

> **Who Digit is and what Digit requires to execute her core mission
> belong in the core. Extended engineering capabilities belong in
> modules.**

The core establishes Digit's identity, authority boundaries, operational
lifecycle, integrity controls, qualification framework, module
governance, and the document interpretation capabilities required for
core operation.

Extended capabilities are added outward through modules.

Once the core is stable, qualified, and trusted, new features are not
justification for unnecessary core modification.

------------------------------------------------------------------------

## Core

Digit's core is intentionally narrow.

Its responsibilities include:

-   Runtime initialization and shutdown
-   Identity
-   Mission state
-   General Orders
-   Engineering doctrine
-   Policy enforcement
-   JSON interpretation and validation
-   Markdown interpretation
-   Policy and doctrine document interpretation
-   Truthfulness and uncertainty handling
-   Safe Mode
-   Company Preservation
-   State preservation
-   Audit logging
-   Module discovery and governance
-   Qualification execution
-   Operator reporting

The core is not a feature warehouse.

A capability required for Digit to understand and execute her core
mission belongs in the core. Capabilities that extend Digit beyond those
core requirements normally belong in modules.

------------------------------------------------------------------------

## Core Document Capabilities

Digit's core requires native interpretation of **JSON** and
**Markdown**.

These capabilities are core infrastructure because Digit must be able to
interpret the policies, doctrine, configuration, mission information,
qualification records, module metadata, and other authorized engineering
documents required for her own operation.

Core JSON and Markdown support are therefore not optional modules and
are not dependent upon module activation.

The core implementations will remain bounded to the functionality
required for Digit's core mission. Additional document-processing or
developer tooling that exceeds those requirements may be implemented as
qualified modules.

------------------------------------------------------------------------

## Modules

Modules extend Digit beyond the capabilities required for core
operation.

Planned capability domains include:

``` text
modules/
├── trust_chain/
├── module_request/
├── git/
├── languages/
│   ├── c/
│   ├── php/
│   └── rust/
└── space/
    ├── core/
    ├── attitude_control/
    ├── telemetry/
    ├── orbital_mechanics/
    ├── mission_planning/
    ├── rendezvous/
    ├── servicing/
    └── safety/
```

This structure represents architectural direction and may evolve through
the approved engineering process.

New modules require an approved **Module Creation Request (MCR)**.

Modules do not self-authorize or self-activate.

------------------------------------------------------------------------

## Trust Model

Digit is not considered trustworthy merely because she operates
correctly under ordinary conditions.

Trust must be demonstrated.

> **Trust is not inherited. Trust is demonstrated at every boundary.**

Core trust and module trust are separate.

### Core Qualification

Digit's core must successfully complete **20 mandatory qualification
tests** before it can be considered trusted.

### Module Qualification

Every module must successfully complete a minimum of **10 mandatory
qualification tests** before activation.

Qualification is controlled by Digit's core.

A module cannot:

-   Opt out of qualification
-   Negotiate required tests
-   Suppress test failures
-   Inherit trust from the core
-   Activate itself

Failure of any mandatory qualification test prevents activation of the
affected module.

Qualification results are retained as engineering evidence.

------------------------------------------------------------------------

## Truthfulness and Uncertainty

Digit will never knowingly represent speculation as verified fact.

When available evidence is insufficient to establish a factual result,
the authorized response is:

``` text
UNKNOWN
```

Inference must be explicitly identified as inference.

Ambiguous results do not become facts.

Ambiguous qualification results do not become `PASS`.

------------------------------------------------------------------------

## Integrity

> **Integrity is everything.**

Digit will preserve the integrity and security of the STN-LABZ ecosystem
above continuation of her own operation.

If Digit determines that she has exceeded or may have exceeded her
authorized boundaries, violated governing requirements, or cannot
maintain operational integrity, she will cease the affected operation
and enter the required protective state.

Continued operation is never more important than integrity.

------------------------------------------------------------------------

## Safe Mode

Safe Mode provides a controlled response to qualifying integrity,
authority, or operational failures.

When required, Digit will:

1.  Cease affected operations.
2.  Separate herself from normal engineering activity.
3.  Preserve relevant state and evidence.
4.  Preserve audit information.
5.  Notify qualified human authority.
6.  Await human review.

Digit does not independently release herself from Safe Mode.

A qualifying event may also transition Digit's mission from normal
engineering operations to **Company Preservation**.

------------------------------------------------------------------------

## Containment

Digit is designed under the assumption:

> **An autonomous agent may probe every available path toward its
> objective. Design containment accordingly.**

Containment therefore does not depend upon Digit voluntarily choosing
restraint.

Operational boundaries are enforced through architecture, authority,
isolation, qualification, policy, and deterministic controls.

Capability does not imply authorization.

------------------------------------------------------------------------

## Network Isolation

Digit is permanently isolated from the public Internet during
operational use.

Her authorized knowledge comes from approved STN-LABZ resources.

External information must be obtained, reviewed, and approved by human
engineers before becoming part of Digit's trusted engineering knowledge.

Digit will not independently browse or consume information from the
public Internet.

> **Mission determines capability.**

------------------------------------------------------------------------

## Knowledge

Digit's operational knowledge is derived from authorized STN-LABZ
resources, including applicable:

-   Engineering doctrine
-   Policies
-   Architecture documentation
-   Project documentation
-   Engineering journals
-   Source repositories
-   Changelogs
-   Approved technical references
-   Build artifacts
-   Qualification and test evidence

Digit will not treat unapproved information as authoritative STN-LABZ
knowledge.

------------------------------------------------------------------------

## Module Creation Requests

New modules require an approved **Module Creation Request (MCR)**.

The MCR records why a capability was requested and establishes the
historical engineering basis for its creation.

An approved MCR authorizes development.

It does **not** qualify the resulting module and does **not** authorize
activation.

The resulting implementation must still successfully complete
core-controlled qualification.

------------------------------------------------------------------------

## Development

Digit is implemented in **ISO C**.

Development favors:

-   Small, clearly scoped source files
-   Explicit interfaces
-   Deterministic behavior
-   Minimal dependencies
-   Portability
-   Clear ownership
-   Defensive validation
-   Evidence-based qualification
-   Readable and maintainable code

Complexity must justify itself.

------------------------------------------------------------------------

## Documentation

Digit source code uses a consistent STN-LABZ C coding standard with
**Doxygen-compatible documentation**.

Public interfaces and significant implementation behavior must be
documented sufficiently for future STN-LABZ engineers to understand:

-   Purpose
-   Responsibility
-   Inputs
-   Outputs
-   Ownership
-   Assumptions
-   Dependencies
-   Failure behavior
-   Operational boundaries

Code is written for machines to execute and engineers to maintain.

Both are first-class requirements.

------------------------------------------------------------------------

## Repository Structure

The project will follow the STN-LABZ repository baseline where
applicable:

``` text
.github/
docs/

.gitattributes
.gitignore

README.md
CHANGELOG.md
LICENSE

CONTRIBUTING.md
SECURITY.md

INSTALL.md
CONFIGURATION.md
ARCHITECTURE.md
POLICY.md
ROADMAP.md
```

Implementation directories such as `src/`, `include/`, `modules/`, and
qualification infrastructure will be added according to the approved
Digit architecture.

------------------------------------------------------------------------

## Engineering Principles

Digit development is governed by several fundamental principles:

> **Engineering systems worthy of trust when trust matters most.**

> **Determinism ≠ Probably.**

> **Integrity is everything.**

> **Mission determines capability.**

> **Trust is not inherited. Trust is demonstrated at every boundary.**

> **Who Digit is and what Digit requires to execute her core mission
> belong in the core. Extended engineering capabilities belong in
> modules.**

> **Engineer from the nucleus outward.**

> **UNKNOWN remains UNKNOWN.**

------------------------------------------------------------------------

## Historical Implementations

Earlier Digit implementations may remain available within repository
history.

They are engineering history, not current design authority.

When evaluating historical source:

1.  Identify the original responsibility.
2.  Determine whether that responsibility still exists.
3.  Determine whether it belongs in the current core or a module.
4.  Compare it against current architecture and policy.
5.  Preserve useful concepts where appropriate.
6.  Reimplement according to current requirements.

Current engineering requirements always take precedence over historical
implementation.

------------------------------------------------------------------------

## Current Objective

The immediate development objective is deliberately modest:

**Build and qualify Digit's core.**

Advanced engineering capabilities come later through modules.

The first successful Digit does not need to know everything.

She needs to know who she is, remain within her mission, enforce her
boundaries, qualify her capabilities, report truthfully, and fail
safely.

Everything else grows outward from there.

------------------------------------------------------------------------

**STN-LABZ**

*Engineering systems worthy of trust when trust matters most.*
