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
