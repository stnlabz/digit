# Architecture

Digit is the STN-LABZ engineering agent. Her core is the governing nucleus;
modules provide separately governed, mission-specific capabilities beyond
foundational core responsibilities.

```text
Digit
├── Core
│   ├── Identity and doctrine
│   ├── General Orders and mission state
│   ├── Markdown and JSON semantic understanding
│   ├── Safe Mode and fail-safe behavior
│   ├── Qualification
│   └── Module governance
└── Modules
    └── Approved mission-specific capabilities
```

The governing principle is:

> Who Digit is belongs in the core. What Digit can do belongs in modules.

Approved policy determines the boundary. Internal source separation does not
convert a core responsibility into a module responsibility.

The working engineering specification is [docs/digit.md](docs/digit.md).

## Out of Scope

### Large Language Models

Digit is not an LLM-based system.

Large Language Model integration is outside the scope of the Digit project.

Digit will not contain, invoke, depend upon, or communicate with an LLM as part of her operational architecture.

Digit's engineering knowledge is derived from authorized STN-LABZ resources through her approved local knowledge and retrieval architecture. When sufficient authoritative information is unavailable, Digit will report `UNKNOWN` rather than substitute unsupported information.
