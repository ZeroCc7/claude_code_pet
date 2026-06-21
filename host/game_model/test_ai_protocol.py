from ai_protocol import ProtocolError, parse_event


def test_parses_status_events_from_all_supported_sources():
    for source in ("codex", "claude_code", "opencode"):
        event = parse_event(
            f'{{"type":"status","source":"{source}",'
            '"task_id":"abc","state":"thinking","timestamp":123}'
        )

        assert event.kind == "status"
        assert event.source == source
        assert event.task_id == "abc"
        assert event.state == "thinking"


def test_parses_successful_completed_task():
    event = parse_event(
        '{"type":"task","source":"codex","task_id":"task-1",'
        '"state":"completed","duration":320,"result":"success"}'
    )

    assert event.kind == "task"
    assert event.duration == 320
    assert event.success


def test_rejects_invalid_events():
    payloads = [
        '{"type":"status","source":"unknown","task_id":"x","state":"thinking"}',
        '{"type":"status","source":"codex","state":"thinking"}',
        '{"type":"status","source":"codex","task_id":"x","state":"mystery"}',
        '{"type":"task","source":"codex","task_id":"x","state":"completed"}',
        "not-json",
    ]
    for payload in payloads:
        try:
            parse_event(payload)
        except ProtocolError:
            continue
        raise AssertionError(f"accepted invalid payload: {payload}")


def test_rejects_messages_over_384_utf8_bytes():
    try:
        parse_event('{"type":"status","padding":"' + "x" * 400 + '"}')
    except ProtocolError as exc:
        assert "too long" in str(exc)
        return
    raise AssertionError("accepted oversized payload")
