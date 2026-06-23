from dataclasses import dataclass
import json


SOURCES = {"codex", "claude_code", "opencode", "codefree_o"}
STATES = {
    "submitted",
    "thinking",
    "tool",
    "editing",
    "waiting",
    "blocked",
    "idle",
}


class ProtocolError(ValueError):
    pass


@dataclass(frozen=True)
class AiEvent:
    kind: str
    source: str
    task_id: str
    state: str
    duration: int = 0
    success: bool = False


def parse_event(payload: str) -> AiEvent:
    if len(payload.encode("utf-8")) > 384:
        raise ProtocolError("message too long")
    try:
        data = json.loads(payload)
    except (json.JSONDecodeError, TypeError) as exc:
        raise ProtocolError("invalid json") from exc
    if not isinstance(data, dict):
        raise ProtocolError("event must be an object")

    kind = data.get("type")
    source = data.get("source")
    task_id = data.get("task_id")
    state = data.get("state")
    if source not in SOURCES:
        raise ProtocolError("unknown source")
    if not isinstance(task_id, str) or not task_id or len(task_id) > 63:
        raise ProtocolError("invalid task_id")

    if kind == "status":
        if state not in STATES:
            raise ProtocolError("unknown state")
        return AiEvent(kind, source, task_id, state)

    if kind == "task":
        duration = data.get("duration")
        result = data.get("result")
        if state != "completed" or not isinstance(duration, int) or duration < 0:
            raise ProtocolError("invalid completed task")
        if result not in {"success", "failure", "cancelled"}:
            raise ProtocolError("invalid result")
        return AiEvent(
            kind,
            source,
            task_id,
            state,
            min(duration, 3600),
            result == "success",
        )

    raise ProtocolError("unknown event type")
