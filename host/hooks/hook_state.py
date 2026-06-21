from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import time
from typing import Callable


SOURCES = {"codex", "claude_code", "opencode"}
STATES = {
    "submitted",
    "thinking",
    "tool",
    "editing",
    "waiting",
    "blocked",
    "idle",
}


class HookState:
    def __init__(
        self, path: Path, clock: Callable[[], float] = time.time
    ) -> None:
        self.path = path
        self.clock = clock

    def begin(self, source: str, session_id: str) -> dict:
        self._validate_source(source)
        now = int(self.clock())
        task_id = hashlib.sha256(
            f"{source}:{session_id}:{now}".encode("utf-8")
        ).hexdigest()[:24]
        data = self._read()
        data[self._key(source, session_id)] = {
            "task_id": task_id,
            "started_at": now,
        }
        self._write(data)
        return self._status_payload(source, task_id, "submitted", now)

    def status(self, source: str, session_id: str, state: str) -> dict:
        self._validate_source(source)
        if state not in STATES:
            raise ValueError(f"unsupported state: {state}")
        record = self._record(source, session_id)
        return self._status_payload(
            source, record["task_id"], state, int(self.clock())
        )

    def complete(
        self, source: str, session_id: str, result: str
    ) -> dict:
        self._validate_source(source)
        if result not in {"success", "failure", "cancelled"}:
            raise ValueError(f"unsupported result: {result}")
        data = self._read()
        key = self._key(source, session_id)
        record = data.get(key)
        if record is None:
            submitted = self.begin(source, session_id)
            data = self._read()
            record = data[key]
            task_id = submitted["task_id"]
        else:
            task_id = record["task_id"]
        elapsed = int(self.clock()) - int(record["started_at"])
        duration = max(60, min(3600, elapsed))
        data.pop(key, None)
        self._write(data)
        return {
            "type": "task",
            "source": source,
            "task_id": task_id,
            "state": "completed",
            "duration": duration,
            "result": result,
        }

    def _record(self, source: str, session_id: str) -> dict:
        data = self._read()
        record = data.get(self._key(source, session_id))
        if record is not None:
            return record
        self.begin(source, session_id)
        return self._read()[self._key(source, session_id)]

    def _read(self) -> dict:
        if not self.path.exists():
            return {}
        try:
            data = json.loads(self.path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError):
            return {}
        return data if isinstance(data, dict) else {}

    def _write(self, data: dict) -> None:
        self.path.parent.mkdir(parents=True, exist_ok=True)
        temporary = self.path.with_suffix(".tmp")
        temporary.write_text(
            json.dumps(data, ensure_ascii=False, separators=(",", ":")),
            encoding="utf-8",
        )
        temporary.replace(self.path)

    @staticmethod
    def _key(source: str, session_id: str) -> str:
        return f"{source}:{session_id or 'default'}"

    @staticmethod
    def _validate_source(source: str) -> None:
        if source not in SOURCES:
            raise ValueError(f"unsupported source: {source}")

    @staticmethod
    def _status_payload(
        source: str, task_id: str, state: str, timestamp: int
    ) -> dict:
        return {
            "type": "status",
            "source": source,
            "task_id": task_id,
            "state": state,
            "timestamp": timestamp,
        }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("action", choices=("begin", "status", "complete"))
    parser.add_argument("--source", required=True, choices=sorted(SOURCES))
    parser.add_argument("--session", default="default")
    parser.add_argument("--state", choices=sorted(STATES))
    parser.add_argument(
        "--result", choices=("success", "failure", "cancelled")
    )
    parser.add_argument(
        "--state-file",
        type=Path,
        default=Path.home() / ".ai-pet-hooks" / "state.json",
    )
    args = parser.parse_args()
    state = HookState(args.state_file)
    if args.action == "begin":
        payload = state.begin(args.source, args.session)
    elif args.action == "status":
        if args.state is None:
            parser.error("--state is required for status")
        payload = state.status(args.source, args.session, args.state)
    else:
        if args.result is None:
            parser.error("--result is required for complete")
        payload = state.complete(args.source, args.session, args.result)
    print(json.dumps(payload, ensure_ascii=False, separators=(",", ":")))


if __name__ == "__main__":
    main()
