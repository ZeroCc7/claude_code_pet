from ai_protocol import ProtocolError, parse_event


def test_parses_start_and_end_for_all_supported_sources():
    for source in ("codex", "claude-code", "opencode", "codefree-o"):
        start = parse_event(f'{{"type":"start","source":"{source}"}}')
        end = parse_event(f'{{"type":"end","source":"{source}"}}')

        assert start.kind == "start"
        assert start.source == source
        assert end.kind == "end"
        assert end.source == source


def test_rejects_old_or_invalid_events():
    payloads = [
        '{"type":"start","source":"unknown"}',
        '{"type":"start"}',
        '{"type":"status","source":"codex"}',
        '{"type":"end","source":"codex","task_id":"old"}',
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
        parse_event('{"type":"start","source":"codex","padding":"' + "x" * 400 + '"}')
    except ProtocolError as exc:
        assert "too long" in str(exc)
        return
    raise AssertionError("accepted oversized payload")
