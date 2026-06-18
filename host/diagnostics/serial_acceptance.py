from __future__ import annotations

import argparse
import time

import serial

REPORT_COMMAND = b"REPORT\n"


class DiagnosticReport:
    def __init__(self) -> None:
        self.saw_boot = False
        self.saw_flash = False
        self.saw_ready = False
        self.errors: list[str] = []

    def consume(self, line: str) -> None:
        line = line.strip()
        if line == "DIAG boot":
            self.saw_boot = True
        elif line.startswith("FLASH "):
            self.saw_flash = True
            required = ("mounted=1", "wrote=1", "verified=1", "cleaned=1")
            if not all(item in line for item in required):
                self.errors.append("flash probe failed")
        elif line == "DIAG ready":
            self.saw_ready = True

    @property
    def complete(self) -> bool:
        return (
            self.saw_boot
            and self.saw_flash
            and self.saw_ready
            and not self.errors
        )


def collect(port: str, timeout: float) -> DiagnosticReport:
    report = DiagnosticReport()
    deadline = time.monotonic() + timeout

    with serial.Serial(port, 115200, timeout=0.25) as device:
        device.reset_input_buffer()
        device.write(REPORT_COMMAND)
        device.flush()
        while time.monotonic() < deadline and not report.complete:
            raw = device.readline()
            if not raw:
                continue
            line = raw.decode("utf-8", errors="replace").strip()
            print(line)
            report.consume(line)

    return report


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", required=True)
    parser.add_argument("--timeout", type=float, default=20.0)
    args = parser.parse_args()

    report = collect(args.port, args.timeout)
    if report.complete:
        print("ACCEPTANCE PASS")
        return 0

    print(f"ACCEPTANCE FAIL errors={report.errors}")
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
