from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys
import time
from typing import Callable, Iterable

from hook_state import HookState, SOURCES, STATES


COMMANDS = tuple(sorted(STATES | {"complete", "failure", "cancelled"}))


def build_payload(
    state: HookState, command: str, source: str, session: str
) -> dict:
    if command == "submitted":
        return state.begin(source, session)
    if command == "complete":
        return state.complete(source, session, "success")
    if command in {"failure", "cancelled"}:
        return state.complete(source, session, command)
    return state.status(source, session, command)


def choose_port(
    explicit: str | None,
    configured: str | None,
    ports: Iterable[dict],
) -> str | None:
    if explicit:
        return explicit
    if configured:
        return configured
    for port in ports:
        if port.get("vid") == 0x2E8A and port.get("pid") == 0x0003:
            return str(port["device"])
    return None


def send_payload(
    payload: dict,
    port: str,
    serial_factory: Callable | None = None,
    timeout_seconds: float = 1.5,
) -> dict:
    if serial_factory is None:
        import serial

        serial_factory = serial.Serial
    device = serial_factory(
        port=port,
        baudrate=115200,
        timeout=0.2,
        write_timeout=0.5,
        dsrdtr=False,
    )
    try:
        encoded = (
            json.dumps(payload, ensure_ascii=False, separators=(",", ":"))
            + "\n"
        ).encode("utf-8")
        device.write(encoded)
        deadline = time.monotonic() + timeout_seconds
        while time.monotonic() < deadline:
            raw = device.readline()
            if not raw:
                continue
            try:
                message = json.loads(raw.decode("utf-8").strip())
            except (UnicodeDecodeError, json.JSONDecodeError):
                continue
            if message.get("type") == "ack":
                return message
    finally:
        device.close()
    raise TimeoutError("设备未返回 ACK")


def read_configured_port(config_path: Path) -> str | None:
    try:
        data = json.loads(config_path.read_text(encoding="utf-8-sig"))
    except (OSError, json.JSONDecodeError):
        return None
    port = data.get("port")
    return str(port) if port else None


def list_serial_ports() -> list[dict]:
    from serial.tools import list_ports

    return [
        {
            "device": item.device,
            "vid": item.vid,
            "pid": item.pid,
        }
        for item in list_ports.comports()
    ]


def main() -> int:
    install_root = Path.home() / ".ai-pet-hooks"
    parser = argparse.ArgumentParser(
        description="手动触发修仙宠物 AI Hook"
    )
    parser.add_argument("command", choices=COMMANDS)
    parser.add_argument("--source", choices=sorted(SOURCES), default="codex")
    parser.add_argument("--session", default="manual")
    parser.add_argument("--port")
    parser.add_argument(
        "--state-file",
        type=Path,
        default=install_root / "state.json",
    )
    args = parser.parse_args()

    try:
        port = choose_port(
            args.port,
            read_configured_port(install_root / "config.json"),
            list_serial_ports(),
        )
        if not port:
            raise RuntimeError("没有找到 RP2040 串口，请使用 --port COM7")
        payload = build_payload(
            HookState(args.state_file),
            args.command,
            args.source,
            args.session,
        )
        ack = send_payload(payload, port)
    except Exception as exc:
        print(f"发送失败：{exc}", file=sys.stderr)
        return 1

    print(
        f"{args.command} -> {port}\n"
        f"{json.dumps(ack, ensure_ascii=False)}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
