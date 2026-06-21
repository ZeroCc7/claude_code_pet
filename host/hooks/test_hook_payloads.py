from hook_state import HookState
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
