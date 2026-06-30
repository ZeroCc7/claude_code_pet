from dataclasses import dataclass
import json


SOURCES = {"codex", "claude-code", "opencode", "codefree-o"}


class ProtocolError(ValueError):
    pass


@dataclass(frozen=True)
class AiEvent:
    kind: str
    source: str


def parse_event(payload: str) -> AiEvent:
    if len(payload.encode("utf-8")) > 384:
        raise ProtocolError("message too long")
    try:
        data = json.loads(payload)
    except (json.JSONDecodeError, TypeError) as exc:
        raise ProtocolError("invalid json") from exc
    if not isinstance(data, dict):
        raise ProtocolError("event must be an object")
    if set(data) != {"type", "source"}:
        raise ProtocolError("invalid fields")
    kind = data["type"]
    source = data["source"]
    if kind not in {"start", "end"}:
        raise ProtocolError("unknown event type")
    if source not in SOURCES:
        raise ProtocolError("unknown source")
    return AiEvent(kind, source)
