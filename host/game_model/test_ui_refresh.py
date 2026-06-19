from pathlib import Path


UI_SOURCE = (
    Path(__file__).parents[2] / "firmware" / "ai_pet" / "game_ui.cpp"
).read_text(encoding="utf-8")


def test_animation_frames_do_not_redraw_the_full_background():
    assert "const bool fullRedraw = dirty_ || force;" in UI_SOURCE
    assert """
  if (fullRedraw) {
    drawInkBackground();
    drawHeader(state.data());
""" in UI_SOURCE


def test_dynamic_frames_restore_only_the_changed_home_regions():
    assert "restoreHomeDynamicRegions();" in UI_SOURCE
