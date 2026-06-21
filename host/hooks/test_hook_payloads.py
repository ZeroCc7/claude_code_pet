from hook_state import HookState
from ai_pet_hook import build_payload, choose_port, send_payload
from pathlib import Path
from tempfile import TemporaryDirectory


def test_begin_creates_stable_task_id_and_submitted_event():
    with TemporaryDirectory() as folder:
        state = HookState(Path(folder) / "state.json", clock=lambda: 1000)

        first = state.begin("codex", "session-1")
        second = state.status("codex", "session-1", "thinking")

        assert first["type"] == "status"
        assert first["state"] == "submitted"
        assert first["task_id"] == second["task_id"]


def test_success_uses_elapsed_time_clamped_to_one_and_sixty_minutes():
    times = iter((1000, 1001, 1000, 9000))
    with TemporaryDirectory() as folder:
        state = HookState(
            Path(folder) / "state.json", clock=lambda: next(times)
        )

        state.begin("codex", "short")
        short = state.complete("codex", "short", "success")
        state.begin("codex", "long")
        long = state.complete("codex", "long", "success")

        assert short["duration"] == 60
        assert long["duration"] == 3600


def test_status_event_has_no_reward_fields():
    with TemporaryDirectory() as folder:
        state = HookState(Path(folder) / "state.json", clock=lambda: 1000)
        state.begin("claude_code", "session")

        event = state.status("claude_code", "session", "editing")

        assert "duration" not in event
        assert "result" not in event


def test_manual_tool_builds_payloads_with_one_session():
    with TemporaryDirectory() as folder:
        state = HookState(Path(folder) / "state.json", clock=lambda: 1000)

        submitted = build_payload(state, "submitted", "codex", "manual")
        editing = build_payload(state, "editing", "codex", "manual")
        completed = build_payload(state, "complete", "codex", "manual")

        assert submitted["task_id"] == editing["task_id"]
        assert completed["task_id"] == submitted["task_id"]
        assert completed["result"] == "success"


def test_manual_tool_prefers_explicit_then_configured_then_rp2040_port():
    ports = [
        {"device": "COM4", "vid": None, "pid": None},
        {"device": "COM7", "vid": 0x2E8A, "pid": 0x0003},
    ]

    assert choose_port("COM9", "COM8", ports) == "COM9"
    assert choose_port(None, "COM8", ports) == "COM8"
    assert choose_port(None, None, ports) == "COM7"


def test_manual_tool_ignores_boot_lines_and_returns_json_ack():
    class FakeSerial:
        def __init__(self):
            self.lines = iter(
                (
                    b"GAME fs=1 save=loaded\n",
                    b'{"type":"ack","status":"accepted"}\n',
                )
            )
            self.written = b""

        def write(self, payload):
            self.written = payload

        def readline(self):
            return next(self.lines, b"")

        def close(self):
            pass

    device = FakeSerial()
    ack = send_payload(
        {"type": "status", "source": "codex", "task_id": "x",
         "state": "thinking", "timestamp": 1},
        "COM7",
        serial_factory=lambda **_: device,
        timeout_seconds=0.2,
    )

    assert device.written.endswith(b"\n")
    assert ack["status"] == "accepted"
