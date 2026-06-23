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


def test_manual_tool_builds_stateless_start_and_end_payloads():
    assert build_payload("start", "codex") == {
        "type": "start",
        "source": "codex",
    }
    assert build_payload("end", "codefree-o") == {
        "type": "end",
        "source": "codefree-o",
    }


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
        {"type": "start", "source": "codex"},
        "COM7",
        serial_factory=lambda **_: device,
        timeout_seconds=0.2,
    )

    assert device.written.endswith(b"\n")
    assert ack["status"] == "accepted"


def test_codex_hook_only_maps_start_and_end():
    for event in ("UserPromptSubmit", "Stop"):
        assert event in CODEX_HOOK_SOURCE
    for removed in ("PreToolUse", "PermissionRequest", "PostToolUse"):
        assert removed not in CODEX_HOOK_SOURCE
    assert '"start"' in CODEX_HOOK_SOURCE
    assert '"end"' in CODEX_HOOK_SOURCE


def test_codex_hook_follows_official_windows_and_stop_contract():
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
                        ],
                        "PreToolUse": [
                            {
                                "hooks": [
                                    {
                                        "type": "command",
                                        "command": "powershell .ai-pet-hooks old",
                                    }
                                ]
                            }
                        ],
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
        for event in ("UserPromptSubmit", "Stop"):
            pet_entries = [
                entry
                for entry in installed["hooks"][event]
                if ".ai-pet-hooks" in json.dumps(entry)
            ]
            assert len(pet_entries) == 1
            handler = pet_entries[0]["hooks"][0]
            assert handler["commandWindows"] == handler["command"]
        for event in ("PreToolUse", "PermissionRequest", "PostToolUse"):
            assert event not in installed["hooks"] or not any(
                ".ai-pet-hooks" in json.dumps(entry)
                for entry in installed["hooks"][event]
            )
