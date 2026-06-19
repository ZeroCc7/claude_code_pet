from pathlib import Path


UI_SOURCE = (
    Path(__file__).parents[2] / "firmware" / "ai_pet" / "game_ui.cpp"
).read_text(encoding="utf-8")
UI_HEADER = (
    Path(__file__).parents[2] / "firmware" / "ai_pet" / "game_ui.h"
).read_text(encoding="utf-8")


def test_animation_frames_do_not_redraw_the_full_background():
    assert (
        "const bool fullRedraw = (dirty_ || force) && page_ == UiPage::Home;"
        in UI_SOURCE
    )
    assert """
  } else if (fullRedraw) {
    drawInkBackground();
    drawHeader(state.data());
""" in UI_SOURCE


def test_dynamic_frames_do_not_clear_pet_region_before_drawing():
    assert "restoreHomeDynamicRegions();" not in UI_SOURCE


def test_pet_frame_is_composited_offscreen_before_one_display_write():
    assert "GFXcanvas16 petCanvas_" in UI_HEADER
    assert "petCanvas_.getBuffer()" in UI_SOURCE
    assert "tft.drawRGBBitmap(kPetRegionX, kPetRegionY" in UI_SOURCE
    assert "pet_.draw(petCanvas_" in UI_SOURCE


def test_operation_pages_are_composited_before_one_display_write():
    assert "GFXcanvas16 menuCanvas_" in UI_HEADER
    assert "drawMenuFrame(state.data());" in UI_SOURCE
    assert "tft.drawRGBBitmap(0, 0, menuCanvas_.getBuffer()" in UI_SOURCE


def test_selection_changes_use_menu_frame_instead_of_direct_full_redraw():
    assert (
        "const bool menuRedraw = (dirty_ || force) && page_ != UiPage::Home;"
        in UI_SOURCE
    )
