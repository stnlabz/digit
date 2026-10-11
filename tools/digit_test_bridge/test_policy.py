"""Negative and positive schema checks; no runtime calls are made."""
import unittest

from policy import BridgeDenied, BridgeIdentity, BridgePolicy, IsolationAttestation


GOOD = BridgeIdentity("test-agent", True, frozenset({"digit:test:read"}), True)
CASE = {"case_id": "social-01", "text": "Digit, how are you?",
        "correlation_id": "run-01"}


class AdmissionTests(unittest.TestCase):
    def setUp(self):
        self.policy = BridgePolicy()

    def denied(self, who, tool, args):
        with self.assertRaises(BridgeDenied):
            self.policy.authorize(who, tool, args)

    def test_no_backend_even_for_valid_request(self):
        self.denied(GOOD, "digit.test_case", CASE)
        self.denied(GOOD, "digit.status", {})

    def test_no_unauthenticated_or_production_user(self):
        self.denied(BridgeIdentity("test-agent", False, GOOD.scopes, True),
                    "digit.test_case", CASE)
        self.denied(BridgeIdentity("operator", True, GOOD.scopes, False),
                    "digit.test_case", CASE)
        self.denied(BridgeIdentity("test-agent", True, frozenset(), True),
                    "digit.test_case", CASE)

    def test_no_mutating_or_unknown_tool(self):
        for name in ("digit.learn", "digit.install", "shell.exec", "digit.ask"):
            with self.subTest(tool=name):
                self.denied(GOOD, name, {})

    def test_reject_extra_args(self):
        self.denied(GOOD, "digit.test_case", {**CASE, "learning": True})
        self.denied(GOOD, "digit.status", {"actor": "other"})

    def test_reject_invalid_identifiers_and_text(self):
        for bad in ("", "../other", "x" * 65):
            self.denied(GOOD, "digit.test_case", {**CASE, "case_id": bad})
        for bad in ("", "\x00", "x" * 2049, "digit\rhello"):
            self.denied(GOOD, "digit.test_case", {**CASE, "text": bad})

    def test_suite_bounds_and_uniqueness(self):
        self.denied(GOOD, "digit.test_suite",
                    {"suite_id": "s", "case_ids": [], "correlation_id": "c"})
        self.denied(GOOD, "digit.test_suite",
                    {"suite_id": "s", "case_ids": ["x", "x"],
                     "correlation_id": "c"})
        self.denied(GOOD, "digit.test_suite",
                    {"suite_id": "s", "case_ids": [str(i) for i in range(17)],
                     "correlation_id": "c"})

    def test_partial_attestation_always_denies(self):
        a = IsolationAttestation(True, True, True, False)
        with self.assertRaises(BridgeDenied):
            BridgePolicy(a).authorize(GOOD, "digit.test_case", CASE)

    def test_complete_attestation_only_passes_admission(self):
        a = IsolationAttestation(True, True, True, True)
        policy = BridgePolicy(a)
        policy.authorize(GOOD, "digit.test_case", CASE)
        policy.authorize(GOOD, "digit.modules", {})
        policy.authorize(GOOD, "digit.test_result", {"receipt_id": "receipt-1"})
        # Admission passing does not invoke anything. No runtime backend exists.


if __name__ == "__main__":
    unittest.main()
