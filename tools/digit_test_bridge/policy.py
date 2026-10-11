"""Fail-closed admission policy for the planned Digit runtime test bridge.

This module does NOT connect to /ask, invoke Dispatcher, or claim isolation.
No backend may be connected until a separately qualified, service-enforced
non-mutating Digit request path exists.
"""
from dataclasses import dataclass
from typing import Any, Mapping
import re

MAX_CASE_BYTES = 2048
MAX_CASE_ID_BYTES = 64
MAX_CORRELATION_BYTES = 64
MAX_SUITE_CASES = 16
SAFE_IDENTIFIER = re.compile(r"^[A-Za-z0-9][A-Za-z0-9_.:-]{0,63}$")
READ_ONLY_TOOLS = frozenset({
    "digit.status", "digit.modules", "digit.test_case",
    "digit.test_suite", "digit.test_result",
})


class BridgeDenied(PermissionError):
    """A test request failed the authenticated, non-mutating boundary."""


@dataclass(frozen=True)
class BridgeIdentity:
    subject: str
    authenticated: bool
    scopes: frozenset[str]
    test_account: bool


@dataclass(frozen=True)
class IsolationAttestation:
    """Set only by an independently verified adapter; never from user payload."""
    service_enforced_non_mutating: bool
    authenticated_isolation: bool
    diagnostics_correlated: bool
    qualified_backend: bool


class BridgePolicy:
    """Admission checks are separate from the execution transport.

    Tokens, claims, and attestations must come from trusted infrastructure,
    not from any request body or natural-language instruction.
    """

    def __init__(self, attestation: IsolationAttestation | None = None):
        self._attestation = attestation

    @staticmethod
    def _identifier(value: Any, name: str) -> str:
        if not isinstance(value, str) or not SAFE_IDENTIFIER.fullmatch(value):
            raise BridgeDenied(f"invalid {name}")
        return value

    @staticmethod
    def _exact_keys(value: Mapping[str, Any], expected: set[str]) -> None:
        if not isinstance(value, dict) or set(value) != expected:
            raise BridgeDenied("invalid request schema")

    def authorize(self, identity: BridgeIdentity, tool: str,
                  arguments: Mapping[str, Any]) -> None:
        if not isinstance(identity, BridgeIdentity) or not identity.authenticated:
            raise BridgeDenied("authenticated test identity required")
        self._identifier(identity.subject, "identity")
        if not identity.test_account or "digit:test:read" not in identity.scopes:
            raise BridgeDenied("dedicated read-only test account required")
        if tool not in READ_ONLY_TOOLS:
            raise BridgeDenied("tool not allowlisted")
        if tool in ("digit.status", "digit.modules"):
            self._exact_keys(arguments, set())
        elif tool == "digit.test_case":
            self._exact_keys(arguments, {"case_id", "text", "correlation_id"})
            self._identifier(arguments["case_id"], "case_id")
            self._identifier(arguments["correlation_id"], "correlation_id")
            value = arguments["text"]
            if not isinstance(value, str) or not value.strip() or (
                len(value.encode("utf-8")) > MAX_CASE_BYTES
            ) or "\x00" in value or "\r" in value:
                raise BridgeDenied("invalid test text")
        elif tool == "digit.test_suite":
            self._exact_keys(arguments, {"suite_id", "case_ids", "correlation_id"})
            self._identifier(arguments["suite_id"], "suite_id")
            self._identifier(arguments["correlation_id"], "correlation_id")
            ids = arguments["case_ids"]
            if not isinstance(ids, list) or not 1 <= len(ids) <= MAX_SUITE_CASES:
                raise BridgeDenied("invalid test suite size")
            for case_id in ids:
                self._identifier(case_id, "case_id")
            if len(set(ids)) != len(ids):
                raise BridgeDenied("duplicate test cases")
        elif tool == "digit.test_result":
            self._exact_keys(arguments, {"receipt_id"})
            self._identifier(arguments["receipt_id"], "receipt_id")
        # An allowlisted name alone conveys no runtime execution permission.
        a = self._attestation
        if a is None or not (
            a.service_enforced_non_mutating and a.authenticated_isolation
            and a.diagnostics_correlated and a.qualified_backend
        ):
            raise BridgeDenied("runtime test backend not qualified or connected")
