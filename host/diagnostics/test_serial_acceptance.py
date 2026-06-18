from serial_acceptance import DiagnosticReport


def test_report_requires_boot_flash_and_ready():
    report = DiagnosticReport()
    report.consume("DIAG boot")
    report.consume("FLASH mounted=1 wrote=1 verified=1 cleaned=1")
    report.consume("DIAG ready")

    assert report.complete
    assert report.errors == []


def test_report_rejects_failed_flash_verification():
    report = DiagnosticReport()
    report.consume("DIAG boot")
    report.consume("FLASH mounted=1 wrote=1 verified=0 cleaned=1")
    report.consume("DIAG ready")

    assert not report.complete
    assert report.errors == ["flash probe failed"]


def test_report_command_is_stable():
    from serial_acceptance import REPORT_COMMAND

    assert REPORT_COMMAND == b"REPORT\n"
