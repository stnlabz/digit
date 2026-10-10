"""Offline safety and routing tests; no Slack or Digit tokens needed."""
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch
from bridge import authorized_reply, load_identity_map, normalize_prompt


class BridgeTests(unittest.TestCase):
    def test_mention_clean(self):
        self.assertEqual(normalize_prompt("<@B01> what is wut?", "B01"), "what is wut?")

    def test_empty_and_overlong_rejected(self):
        self.assertIsNone(normalize_prompt("<@B01>", "B01"))
        self.assertIsNone(normalize_prompt("x" * 4000, "B01"))

    def test_unmapped_actor_denied(self):
        outputs = []
        authorized_reply({"user": "U99", "text": "<@B01> hello", "ts": "1"}, "B01", {}, "https://example.org", lambda **kw: outputs.append(kw))
        self.assertEqual(outputs, [])

    def test_channel_without_mention_denied(self):
        outputs = []
        authorized_reply({"user": "U01", "text": "hello", "ts": "1"}, "B01", {"U01": {"username": "a", "password": "b"}}, "https://example.org", lambda **kw: outputs.append(kw))
        self.assertEqual(outputs, [])

    def test_bot_message_denied(self):
        outputs = []
        authorized_reply({"user": "U01", "bot_id": "BOT", "text": "<@B01> hello"}, "B01", {"U01": {"username": "a", "password": "b"}}, "https://example.org", lambda **kw: outputs.append(kw))
        self.assertEqual(outputs, [])

    def test_authorized_thread(self):
        outputs = []
        with patch("bridge.ask_digit", return_value="Grounded answer") as ask:
            authorized_reply({"user": "U01", "text": "<@B01> hello", "ts": "23", "thread_ts": "22"}, "B01", {"U01": {"username": "a", "password": "b"}}, "https://example.org", lambda **kw: outputs.append(kw))
        self.assertEqual(outputs, [{"text": "Grounded answer", "thread_ts": "22"}])
        ask.assert_called_once()

    def test_private_mapping_permissions(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "identities.json"
            path.write_text('{"U01":{"username":"digit_user","password":"secret"}}')
            path.chmod(0o600)
            self.assertIn("U01", load_identity_map(str(path)))
            path.chmod(0o644)
            with self.assertRaises(ValueError):
                load_identity_map(str(path))


if __name__ == "__main__":
    unittest.main()
