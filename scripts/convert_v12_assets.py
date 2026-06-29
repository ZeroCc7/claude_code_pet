from __future__ import annotations

from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]

SCENE_SOURCE = ROOT / "assets/raw/backgrounds/bamboo_realm_scene.png"
SCENE_PREVIEW = ROOT / "assets/processed/backgrounds/bamboo_realm_scene_128x54.png"
SCENE_HEADER = ROOT / "firmware/ai_pet/assets/bamboo_realm_scene.h"

BOSS_SOURCE = ROOT / "assets/raw/bosses/bamboo_guardian_2x2.png"
BOSS_PREVIEW = ROOT / "assets/processed/creatures/bamboo_guardian"
BOSS_HEADER = ROOT / "firmware/ai_pet/assets/bamboo_guardian.h"

ITEM_SOURCES = (
    ("QingyunSword", ROOT / "assets/raw/items/qingyun_sword.png"),
    ("SpiritBambooJade", ROOT / "assets/raw/items/spirit_bamboo_jade.png"),
)
ITEM_PREVIEW = ROOT / "assets/processed/items"
ITEM_HEADER = ROOT / "firmware/ai_pet/assets/region_treasures.h"

SCENE_WIDTH = 128
SCENE_HEIGHT = 54
BOSS_SIZE = 48
BOSS_LARGE_SIZE = 72
BOSS_FRAMES = 4
ITEM_SIZE = 24
ALPHA_THRESHOLD = 96


def rgb565(red: int, green: int, blue: int) -> int:
    return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3)


def append_array(lines: list[str], kind: str, name: str, values: list[int]) -> None:
    width = 4 if kind == "uint16_t" else 2
    lines.append(f"const {kind} {name}[] PROGMEM = {{")
    for offset in range(0, len(values), 14):
        chunk = values[offset : offset + 14]
        lines.append(
            "  " + ", ".join(f"0x{value:0{width}X}" for value in chunk) + ","
        )
    lines.extend(("};", ""))


def remove_magenta(image: Image.Image) -> Image.Image:
    rgba = image.convert("RGBA")
    for y in range(rgba.height):
        for x in range(rgba.width):
            red, green, blue, alpha = rgba.getpixel((x, y))
            chroma = red > 210 and blue > 180 and green < 90
            rgba.putpixel((x, y), (red, green, blue, 0 if chroma else alpha))
    return rgba


def masked_frame_data(image: Image.Image) -> tuple[list[int], list[int]]:
    pixels = [
        rgb565(*image.getpixel((x, y))[:3])
        for y in range(image.height)
        for x in range(image.width)
    ]
    mask: list[int] = []
    for y in range(image.height):
        for byte_x in range(0, image.width, 8):
            value = 0
            for bit in range(8):
                x = byte_x + bit
                alpha = image.getpixel((x, y))[3] if x < image.width else 0
                if alpha >= ALPHA_THRESHOLD:
                    value |= 0x80 >> bit
            mask.append(value)
    return pixels, mask


def normalize_subject(image: Image.Image, size: int, bottom_anchor: bool) -> Image.Image:
    rgba = remove_magenta(image)
    bbox = rgba.getchannel("A").getbbox()
    if bbox is None:
        raise ValueError("empty asset frame")
    subject = rgba.crop(bbox)
    scale = min((size - 2) / subject.width, (size - 2) / subject.height)
    resized_size = (
        max(1, round(subject.width * scale)),
        max(1, round(subject.height * scale)),
    )
    subject = subject.resize(resized_size, Image.Resampling.NEAREST)
    frame = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    y = size - resized_size[1] - 1 if bottom_anchor else (size - resized_size[1]) // 2
    frame.alpha_composite(subject, ((size - resized_size[0]) // 2, y))
    return frame


def convert_scene() -> None:
    source = Image.open(SCENE_SOURCE).convert("RGB")
    target_ratio = SCENE_WIDTH / SCENE_HEIGHT
    crop_width = min(source.width, round(source.height * target_ratio))
    left = max(0, (source.width - crop_width) // 2)
    cropped = source.crop((left, 0, left + crop_width, source.height))
    resized = cropped.resize((SCENE_WIDTH, SCENE_HEIGHT), Image.Resampling.LANCZOS)
    preview = resized.quantize(
        colors=64,
        method=Image.Quantize.MEDIANCUT,
        dither=Image.Dither.NONE,
    ).convert("RGB")
    SCENE_PREVIEW.parent.mkdir(parents=True, exist_ok=True)
    preview.save(SCENE_PREVIEW)
    pixels = [
        rgb565(*preview.getpixel((x, y)))
        for y in range(SCENE_HEIGHT)
        for x in range(SCENE_WIDTH)
    ]
    lines = [
        "#pragma once",
        "",
        "#include <Arduino.h>",
        "",
        f"constexpr uint8_t kBambooRealmSceneWidth = {SCENE_WIDTH};",
        f"constexpr uint8_t kBambooRealmSceneHeight = {SCENE_HEIGHT};",
        "",
    ]
    append_array(lines, "uint16_t", "kBambooRealmScene", pixels)
    SCENE_HEADER.write_text("\n".join(lines), encoding="utf-8")


def convert_boss() -> None:
    source = Image.open(BOSS_SOURCE).convert("RGB")
    cell_width = source.width // 2
    cell_height = source.height // 2
    BOSS_PREVIEW.mkdir(parents=True, exist_ok=True)
    lines = [
        "#pragma once",
        "",
        "#include <Arduino.h>",
        "",
        f"constexpr uint8_t kBambooGuardianWidth = {BOSS_SIZE};",
        f"constexpr uint8_t kBambooGuardianHeight = {BOSS_SIZE};",
        f"constexpr uint8_t kBambooGuardianFrameCount = {BOSS_FRAMES};",
        "",
    ]
    all_pixels: list[list[int]] = []
    all_masks: list[list[int]] = []
    large_frame: Image.Image | None = None
    for frame_index in range(BOSS_FRAMES):
        column = frame_index % 2
        row = frame_index // 2
        cell = source.crop(
            (
                column * cell_width,
                row * cell_height,
                (column + 1) * cell_width,
                (row + 1) * cell_height,
            )
        )
        frame = normalize_subject(cell, BOSS_SIZE, bottom_anchor=True)
        frame.save(BOSS_PREVIEW / f"idle-{frame_index + 1}.png")
        pixels, mask = masked_frame_data(frame)
        all_pixels.append(pixels)
        all_masks.append(mask)
        if frame_index == 0:
            large_frame = normalize_subject(cell, BOSS_LARGE_SIZE, bottom_anchor=True)
            large_frame.save(BOSS_PREVIEW / "large.png")

    for frame_index in range(BOSS_FRAMES):
        append_array(
            lines,
            "uint16_t",
            f"kBambooGuardianFrame{frame_index}Pixels",
            all_pixels[frame_index],
        )
        append_array(
            lines,
            "uint8_t",
            f"kBambooGuardianFrame{frame_index}Mask",
            all_masks[frame_index],
        )
    lines.append("const uint16_t* const kBambooGuardianPixels[] PROGMEM = {")
    for i in range(BOSS_FRAMES):
        lines.append(f"  kBambooGuardianFrame{i}Pixels,")
    lines.extend(("};", ""))
    lines.append("const uint8_t* const kBambooGuardianMasks[] PROGMEM = {")
    for i in range(BOSS_FRAMES):
        lines.append(f"  kBambooGuardianFrame{i}Mask,")
    lines.extend(("};", ""))

    if large_frame is None:
        raise ValueError("missing large bamboo guardian frame")
    large_pixels, large_mask = masked_frame_data(large_frame)
    lines.extend((
        f"constexpr uint8_t kBambooGuardianLargeWidth = {BOSS_LARGE_SIZE};",
        f"constexpr uint8_t kBambooGuardianLargeHeight = {BOSS_LARGE_SIZE};",
        "",
    ))
    append_array(lines, "uint16_t", "kBambooGuardianLargePixels", large_pixels)
    append_array(lines, "uint8_t", "kBambooGuardianLargeMask", large_mask)
    BOSS_HEADER.write_text("\n".join(lines), encoding="utf-8")


def convert_items() -> None:
    ITEM_PREVIEW.mkdir(parents=True, exist_ok=True)
    lines = [
        "#pragma once",
        "",
        "#include <Arduino.h>",
        "",
        f"constexpr uint8_t kRegionTreasureIconWidth = {ITEM_SIZE};",
        f"constexpr uint8_t kRegionTreasureIconHeight = {ITEM_SIZE};",
        "",
        "struct RegionTreasureIcon {",
        "  const uint16_t* pixels;",
        "  const uint8_t* mask;",
        "};",
        "",
    ]
    definitions: list[tuple[str, str, str]] = []
    for name, path in ITEM_SOURCES:
        frame = normalize_subject(Image.open(path), ITEM_SIZE, bottom_anchor=False)
        frame.save(ITEM_PREVIEW / f"{name}.png")
        pixels, mask = masked_frame_data(frame)
        pixel_name = f"kRegionTreasure{name}Pixels"
        mask_name = f"kRegionTreasure{name}Mask"
        append_array(lines, "uint16_t", pixel_name, pixels)
        append_array(lines, "uint8_t", mask_name, mask)
        definitions.append((name, pixel_name, mask_name))
    for name, pixels, mask in definitions:
        lines.append(
            f"const RegionTreasureIcon kRegionTreasure{name} = "
            f"{{{pixels}, {mask}}};"
        )
    lines.append("")
    ITEM_HEADER.write_text("\n".join(lines), encoding="utf-8")


def main() -> None:
    convert_scene()
    convert_boss()
    convert_items()
    print("V1.2 assets converted")


if __name__ == "__main__":
    main()
