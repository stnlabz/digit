"""Digit Slack Socket Mode bridge. No Core changes, no inferred Slack authority."""
import json
import logging
import os
import re
import stat
import threading
import urllib.error
import urllib.request
from pathlib import Path

from slack_bolt import App
from slack_bolt.adapter.socket_mode import SocketModeHandler

LOG = logging.getLogger("digit-slack")
MENTION = re.compile(r"<@[A-Z0-9]+>")
USER_ID = re.compile(r"^U[A-Z0-9]+$")
MAX_PROMPT = 4000


def load_identity_map(filename):
    """Private operator-managed mapping: Slack user ID -> Digit username/password."""
    path = Path(filename)
    mode = path.stat()
    if not stat.S_ISREG(mode.st_mode) or mode.st_mode & 0o077:
        raise ValueError("Identity mapping must be a private regular file (0600)")
    if mode.st_uid != os.geteuid():
        raise ValueError("Identity mapping must belong to the bridge service user")
    mapping = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(mapping, dict):
        raise ValueError("Identity mapping must be an object")
    for slack_id, identity in mapping.items():
        if not USER_ID.fullmatch(slack_id):
            raise ValueError("Invalid Slack identity key")
        if not isinstance(identity, dict) or not all(
            isinstance(identity.get(key), str) and identity[key]
            for key in ("username", "password")
        ):
            raise ValueError("Every Slack identity needs a Digit username and password")
    return mapping


def digit_request(url, path, body, bearer=None):
    if not url.startswith("https://"):
        raise ValueError("Digit Interface requires trusted HTTPS")
    data = body.encode("utf-8")
    headers = {"Content-Type": "text/plain; charset=utf-8"}
    if bearer:
        headers["Authorization"] = "Bearer " + bearer
    request = urllib.request.Request(
        url.rstrip("/") + path, data=data, headers=headers, method="POST"
    )
    with urllib.request.urlopen(request, timeout=40) as response:
        if response.status != 200:
            raise ValueError("Digit Interface returned non-success status")
        return json.loads(response.read(65536).decode("utf-8"))


def ask_digit(url, identity, prompt):
    """Independent authenticated request per Slack actor; never fall back to SA."""
    username = identity["username"]
    password = identity["password"]
    if any(c in username + password for c in ("\t", "\r", "\n")):
        raise ValueError("Invalid account credential field")
    auth = digit_request(url, "/session/login", username + "\t" + password)
    token = auth.get("token", "")
    if not isinstance(token, str) or len(token) != 64:
        raise ValueError("Digit authentication failed")
    try:
        answer = digit_request(url, "/ask", prompt, token)
        result = answer.get("answer")
        if not isinstance(result, str) or not result:
            raise ValueError("Digit returned no grounded answer")
        return result[:3500]
    finally:
        try:
            digit_request(url, "/session/logout", "", token)
        except Exception:
            LOG.warning("Digit logout failed; review session cleanup")


def normalize_prompt(text, bot_user_id):
    stripped = text.replace("<@" + bot_user_id + ">", " ")
    stripped = MENTION.sub(" ", stripped).strip()
    if not stripped or len(stripped.encode("utf-8")) >= MAX_PROMPT:
        return None
    return stripped


def authorized_reply(event, bot_user_id, mapping, url, say):
    """Only direct app mentions or DMs, no bot loops, no implicit access."""
    user = event.get("user", "")
    if event.get("bot_id") or event.get("subtype") or user not in mapping:
        return
    channel_type = event.get("channel_type", "")
    text = event.get("text", "")
    if channel_type != "im" and "<@" + bot_user_id + ">" not in text:
        return
    prompt = normalize_prompt(text, bot_user_id)
    if not prompt:
        return
    try:
        answer = ask_digit(url, mapping[user], prompt)
    except Exception:
        LOG.exception("Digit request failed for authenticated Slack event")
        answer = "Digit could not complete that request. No permissions were changed."
    say(text=answer, thread_ts=event.get("thread_ts") or event.get("ts"))


def main():
    logging.basicConfig(level=logging.INFO)
    app_token = os.environ["SLACK_APP_TOKEN"]
    bot_token = os.environ["SLACK_BOT_TOKEN"]
    url = os.environ["DIGIT_INTERFACE_URL"]
    mapping = load_identity_map(os.environ["DIGIT_SLACK_IDENTITIES"])
    if not url.startswith("https://"):
        raise SystemExit("DIGIT_INTERFACE_URL must use HTTPS")
    app = App(token=bot_token)
    bot_user_id = app.client.auth_test()["user_id"]

    @app.event("app_mention")
    def mention(event, say):
        authorized_reply(event, bot_user_id, mapping, url, say)

    @app.event("message")
    def private_message(event, say):
        if event.get("channel_type") == "im":
            authorized_reply(event, bot_user_id, mapping, url, say)

    LOG.info("Digit Slack bridge starting (explicit identities: %d)", len(mapping))
    SocketModeHandler(app, app_token).start()


if __name__ == "__main__":
    main()
