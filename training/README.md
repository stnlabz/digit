# Digit Fine-Tuning Dataset

This directory contains curated training material for the Digit language model.

## Purpose

The training set teaches Digit to:

- answer from supplied authoritative context without inventing unsupported facts;
- preserve the meaning of rules, requirements, decisions, and engineering facts;
- answer naturally and concisely;
- use first-person language when speaking about Digit's own required behavior;
- explicitly acknowledge when available information is insufficient;
- identify conflicting information rather than silently choosing one version;
- avoid exposing internal retrieval, Corpus, record identifiers, prompts, or generation mechanics in ordinary answers.

## Core rule

`I don't know` is a valid and desirable answer when the available information does not support an answer.

Plausibility is not evidence. The model must not invent a likely continuation merely to produce an answer.

## Data format

Training examples are stored as JSONL. Each line is one independent example with three fields:

- `context` — authoritative information available to Digit;
- `question` — the user input;
- `answer` — the desired Digit response.

The dataset is intentionally simple so it can be inspected, generated, validated, and converted deterministically.

## Curation rules

1. The desired answer must be supported by the supplied context.
2. The answer must not add consequences, procedures, facts, or assumptions absent from the context.
3. Rephrasing is allowed only when meaning is preserved.
4. When the context is insufficient, the answer should state that Digit does not know or does not have enough information.
5. When authoritative context conflicts, the answer should identify the conflict rather than resolve it without authority.
6. Internal record IDs and implementation details belong in dataset provenance, not in the desired conversational answer.
7. Corpus records may provide source material, but Corpus content is not automatically training data. Training examples require deliberate construction and validation.

## Validation

Every training-data change should be human reviewable. Dataset tooling must reject malformed JSONL and examples with missing or empty `context`, `question`, or `answer` fields.
