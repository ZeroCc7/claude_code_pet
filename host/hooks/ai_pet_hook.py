from __future__ import annotations

import argparse
import contextlib
import json
from pathlib import Path
import sys
import time
from typing import Callable, Iterable

try:
    import fcntl

    @contextlib.contextmanager
    def _serial_lock(path: Path, timeout: float = 3.0):
        path.parent.mkdir(parents=True, exist_ok=True)
        fd = open(path, "w")
        try:
            fcntl.flock(fd, fcntl.LOCK_EX)
            yield
        finally:
            fcntl.flock(fd, fcntl.LOCK_UN)
            fd.close()

except ImportError:
    import msvcrt

    @contextlib.contextmanager
    def _serial_lock(path: Path, timeout: float = 3.0):
        path.parent.mkdir(parents=True, exist_ok=True)
        fd = open(path, "w")
        try:
            deadline = time.monotonic() + timeout
            locked = False
            while time.monotonic() < deadline:
                try:
                    msvcrt.locking(fd.fileno(), msvcrt.LK_NBLCK, 1)
                    locked = True
                    break
                except OSError:
                    time.sleep(0.05)
            if not locked:
                raise TimeoutError("无法获取串口锁")
            yield
        finally:
            if locked:
                try:
                    fd.seek(0)
                    msvcrt.locking(fd.fileno(), msvcrt.LK_UNLCK, 1)
                except OSError:
                    pass
            fd.close()

SOURCES = {"codex", "claude-code", "opencode", "codefree-o"}
COMMANDS = ("start", "end")


def build_payload(command: str, source: str) -> dict:
    if command not in COMMANDS or source not in SOURCES:
        raise ValueError("unsupported event")
    return {"type": command, "source": source}


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


def handle_send(args: argparse.Namespace) -> int:
    install_root = Path.home() / ".ai-pet-hooks"
    lock_path = install_root / "serial.lock"
    try:
        port = choose_port(
            args.port,
            read_configured_port(install_root / "config.json"),
            list_serial_ports(),
        )
        if not port:
            raise RuntimeError("没有找到 RP2040 串口，请使用 --port COM7")
        raw = sys.stdin.read().strip()
        if not raw:
            raise RuntimeError("未收到 payload")
        payload = json.loads(raw)
        with _serial_lock(lock_path):
            ack = send_payload(payload, port)
    except Exception as exc:
        print(f"发送失败：{exc}", file=sys.stderr)
        return 1
    print(json.dumps(ack, ensure_ascii=False))
    return 0


def main() -> int:
    install_root = Path.home() / ".ai-pet-hooks"
    parser = argparse.ArgumentParser(
        description="手动触发修仙宠物 AI Hook"
    )
    sub = parser.add_subparsers(dest="subcommand")

    send_parser = sub.add_parser("send", help="从 stdin 读取 JSON 并发送到设备")
    send_parser.add_argument("--port")

    parser.add_argument("command", nargs="?", choices=COMMANDS)
    parser.add_argument("--source", choices=sorted(SOURCES), default="codex")
    parser.add_argument("--port")
    args = parser.parse_args()

    if args.subcommand == "send":
        return handle_send(args)

    if not args.command:
        parser.error("command is required")

    try:
        port = choose_port(
            args.port,
            read_configured_port(install_root / "config.json"),
            list_serial_ports(),
        )
        if not port:
            raise RuntimeError("没有找到 RP2040 串口，请使用 --port COM7")
        payload = build_payload(args.command, args.source)
        with _serial_lock(install_root / "serial.lock"):
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
