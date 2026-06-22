from hook_state import HookState
from ai_pet_hook import build_payload, choose_port, send_payload
from pathlib import Path
import json
import subprocess
from tempfile import TemporaryDirectory


ROOT = Path(__file__).parents[2]
CODEX_HOOK_SOURCE = (ROOT / "host" / "hooks" / "codex-hook.ps1").read_text(
    encoding="utf-8"
)
INSTALLER = ROOT / "scripts" / "install-ai-hooks.ps1"


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


def test_codex_hook_maps_lifecycle_events_and_editing_tools():
    for event in (
        "UserPromptSubmit",
        "PreToolUse",
        "PermissionRequest",
        "PostToolUse",
        "Stop",
    ):
        assert event in CODEX_HOOK_SOURCE
    assert "apply_patch|Edit|Write" in CODEX_HOOK_SOURCE
    assert '"submitted"' in CODEX_HOOK_SOURCE
    assert '"editing"' in CODEX_HOOK_SOURCE
    assert '"tool"' in CODEX_HOOK_SOURCE
    assert '"waiting"' in CODEX_HOOK_SOURCE
    assert '"thinking"' in CODEX_HOOK_SOURCE
    assert '"complete"' in CODEX_HOOK_SOURCE


def test_codex_hook_follows_official_windows_and_stop_contract():
    assert "$payload.session_id" in CODEX_HOOK_SOURCE
    assert "$payload.turn_id" in CODEX_HOOK_SOURCE
    assert "$payload.tool_name" in CODEX_HOOK_SOURCE
    assert 'if ($Event -eq "Stop")' in CODEX_HOOK_SOURCE
    assert "Write-Output \"{}\"" in CODEX_HOOK_SOURCE


def test_codex_only_installer_preserves_notify_and_is_idempotent():
    with TemporaryDirectory() as folder:
        profile = Path(folder)
        codex_home = profile / ".codex"
        codex_home.mkdir()
        config = codex_home / "config.toml"
        original_config = 'notify = ["existing-notifier", "turn-ended"]\n'
        config.write_text(original_config, encoding="utf-8")
        hooks_path = codex_home / "hooks.json"
        hooks_path.write_text(
            json.dumps(
                {
                    "hooks": {
                        "SessionStart": [
                            {
                                "hooks": [
                                    {
                                        "type": "command",
                                        "command": "existing-command",
                                    }
                                ]
                            }
                        ]
                    }
                }
            ),
            encoding="utf-8",
        )
        command = [
            "powershell",
            "-NoProfile",
            "-ExecutionPolicy",
            "Bypass",
            "-File",
            str(INSTALLER),
            "-Target",
            "Codex",
            "-Port",
            "COM99",
            "-UserProfileRoot",
            str(profile),
            "-SkipUserEnvironment",
        ]

        subprocess.run(command, check=True, capture_output=True, text=True)
        subprocess.run(command, check=True, capture_output=True, text=True)

        assert config.read_text(encoding="utf-8") == original_config
        assert not hooks_path.read_bytes().startswith(b"\xef\xbb\xbf")
        installed = json.loads(hooks_path.read_text(encoding="utf-8-sig"))
        assert installed["hooks"]["SessionStart"][0]["hooks"][0][
            "command"
        ] == "existing-command"
        for event in (
            "UserPromptSubmit",
            "PreToolUse",
            "PermissionRequest",
            "PostToolUse",
            "Stop",
        ):
            pet_entries = [
                entry
                for entry in installed["hooks"][event]
                if ".ai-pet-hooks" in json.dumps(entry)
            ]
            assert len(pet_entries) == 1
            handler = pet_entries[0]["hooks"][0]
            assert handler["commandWindows"] == handler["command"]
