from __future__ import annotations

from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[2]
SCENE_SOURCE = ROOT / "assets/raw/backgrounds/qingyun_scene.png"
SCENE_PREVIEW = ROOT / "assets/processed/backgrounds/qingyun_scene_128x54.png"
SCENE_HEADER = ROOT / "firmware/ai_pet/assets/qingyun_scene.h"
ICON_SOURCE = ROOT / "assets/raw/ui/qingyun_icons/icons_3x3.png"
ICON_PREVIEW = ROOT / "assets/processed/ui/qingyun_icons/firmware_preview"
ICON_HEADER = ROOT / "firmware/ai_pet/assets/qingyun_ui_icons.h"
PET_SOURCE = ROOT / "assets/processed/pets/firmware_preview"
PET_PREVIEW = ROOT / "assets/processed/pets/qingyun_preview"
PET_HEADER = ROOT / "firmware/ai_pet/assets/qingyun_pets.h"
SCENE_WIDTH = 128
SCENE_HEIGHT = 54
ICON_SIZE = 18
PET_SIZE = 32
ICON_NAMES = (
    "SpiritHerb",
    "RecoveryPill",
    "AttackTalisman",
    "GuardTalisman",
    "Token",
    "DemonBeast",
    "WoundedCultivator",
    "Shortcut",
    "Energy",
)
PET_NAMES = (
    "egg",
    "rookie_a",
    "rookie_b",
    "final_a1",
    "final_a2",
    "final_b1",
    "final_b2",
)


def rgb565(red: int, green: int, blue: int) -> int:
    return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3)


def append_array(
    lines: list[str], kind: str, name: str, values: list[int]
) -> None:
    width = 4 if kind == "uint16_t" else 2
    lines.append(f"const {kind} {name}[] PROGMEM = {{")
    for offset in range(0, len(values), 14):
        chunk = values[offset : offset + 14]
        lines.append(
            "  " + ", ".join(
                f"0x{value:0{width}X}" for value in chunk
            ) + ","
        )
    lines.extend(("};", ""))


def convert_scene() -> None:
    source = Image.open(SCENE_SOURCE).convert("RGB")
    target_ratio = SCENE_WIDTH / SCENE_HEIGHT
    crop_width = min(source.width, round(source.height * target_ratio))
    left = max(0, (source.width - crop_width) // 2)
    cropped = source.crop((left, 0, left + crop_width, source.height))
    resized = cropped.resize(
        (SCENE_WIDTH, SCENE_HEIGHT), Image.Resampling.LANCZOS
    )
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
        f"constexpr uint8_t kQingyunSceneWidth = {SCENE_WIDTH};",
        f"constexpr uint8_t kQingyunSceneHeight = {SCENE_HEIGHT};",
        "",
    ]
    append_array(lines, "uint16_t", "kQingyunScene", pixels)
    SCENE_HEADER.write_text("\n".join(lines), encoding="utf-8")


def remove_magenta(image: Image.Image) -> Image.Image:
    rgba = image.convert("RGBA")
    for y in range(rgba.height):
        for x in range(rgba.width):
            red, green, blue, _ = rgba.getpixel((x, y))
            chroma = red > 210 and blue > 180 and green < 90
            rgba.putpixel((x, y), (red, green, blue, 0 if chroma else 255))
    return rgba


def normalize_icon(image: Image.Image) -> Image.Image:
    bbox = image.getchannel("A").getbbox()
    if bbox is None:
        raise ValueError("empty Qingyun icon")
    subject = image.crop(bbox)
    scale = min((ICON_SIZE - 1) / subject.width,
                (ICON_SIZE - 1) / subject.height)
    size = (
        max(1, round(subject.width * scale)),
        max(1, round(subject.height * scale)),
    )
    subject = subject.resize(size, Image.Resampling.NEAREST)
    frame = Image.new("RGBA", (ICON_SIZE, ICON_SIZE), (0, 0, 0, 0))
    frame.alpha_composite(
        subject,
        ((ICON_SIZE - size[0]) // 2, (ICON_SIZE - size[1]) // 2),
    )
    return frame


def convert_icons() -> None:
    source = Image.open(ICON_SOURCE).convert("RGB")
    cell_width = source.width // 3
    cell_height = source.height // 3
    ICON_PREVIEW.mkdir(parents=True, exist_ok=True)
    lines = [
        "#pragma once",
        "",
        "#include <Arduino.h>",
        "",
        f"constexpr uint8_t kQingyunIconWidth = {ICON_SIZE};",
        f"constexpr uint8_t kQingyunIconHeight = {ICON_SIZE};",
        "",
        "struct QingyunUiIcon {",
        "  const uint16_t* pixels;",
        "  const uint8_t* mask;",
        "};",
        "",
    ]
    definitions = []
    for index, name in enumerate(ICON_NAMES):
        column = index % 3
        row = index // 3
        cell = source.crop(
            (
                column * cell_width,
                row * cell_height,
                (column + 1) * cell_width,
                (row + 1) * cell_height,
            )
        )
        frame = normalize_icon(remove_magenta(cell))
        frame.save(ICON_PREVIEW / f"{index + 1}-{name.lower()}.png")
        pixels = [
            rgb565(*frame.getpixel((x, y))[:3])
            for y in range(ICON_SIZE)
            for x in range(ICON_SIZE)
        ]
        mask = []
        for y in range(ICON_SIZE):
            for byte_x in range(0, ICON_SIZE, 8):
                value = 0
                for bit in range(8):
                    x = byte_x + bit
                    alpha = frame.getpixel((x, y))[3] if x < ICON_SIZE else 0
                    if alpha >= 96:
                        value |= 0x80 >> bit
                mask.append(value)
        pixel_name = f"kQingyunIcon{name}Pixels"
        mask_name = f"kQingyunIcon{name}Mask"
        append_array(lines, "uint16_t", pixel_name, pixels)
        append_array(lines, "uint8_t", mask_name, mask)
        definitions.append((name, pixel_name, mask_name))
    for name, pixels, mask in definitions:
        lines.append(
            f"const QingyunUiIcon kQingyunIcon{name} = "
            f"{{{pixels}, {mask}}};"
        )
    lines.append("")
    ICON_HEADER.write_text("\n".join(lines), encoding="utf-8")


def convert_pets() -> None:
    PET_PREVIEW.mkdir(parents=True, exist_ok=True)
    lines = [
        "#pragma once",
        "",
        "#include <Arduino.h>",
        "",
        f"constexpr uint8_t kQingyunPetWidth = {PET_SIZE};",
        f"constexpr uint8_t kQingyunPetHeight = {PET_SIZE};",
        "",
        "struct QingyunPetSprite {",
        "  const uint16_t* pixels;",
        "  const uint8_t* mask;",
        "};",
        "",
    ]
    definitions = []
    for name in PET_NAMES:
        source = Image.open(PET_SOURCE / name / "frame-1.png").convert("RGBA")
        bbox = source.getchannel("A").getbbox()
        if bbox is None:
            raise ValueError(f"empty pet preview: {name}")
        subject = source.crop(bbox)
        scale = min((PET_SIZE - 2) / subject.width,
                    (PET_SIZE - 2) / subject.height)
        size = (
            max(1, round(subject.width * scale)),
            max(1, round(subject.height * scale)),
        )
        subject = subject.resize(size, Image.Resampling.NEAREST)
        frame = Image.new("RGBA", (PET_SIZE, PET_SIZE), (0, 0, 0, 0))
        frame.alpha_composite(
            subject,
            ((PET_SIZE - size[0]) // 2, PET_SIZE - size[1] - 1),
        )
        frame.save(PET_PREVIEW / f"{name}.png")
        pixels = [
            rgb565(*frame.getpixel((x, y))[:3])
            for y in range(PET_SIZE)
            for x in range(PET_SIZE)
        ]
        mask = []
        for y in range(PET_SIZE):
            for byte_x in range(0, PET_SIZE, 8):
                value = 0
                for bit in range(8):
                    x = byte_x + bit
                    if frame.getpixel((x, y))[3] >= 96:
                        value |= 0x80 >> bit
                mask.append(value)
        symbol = "".join(part.title() for part in name.split("_"))
        pixel_name = f"kQingyunPet{symbol}Pixels"
        mask_name = f"kQingyunPet{symbol}Mask"
        append_array(lines, "uint16_t", pixel_name, pixels)
        append_array(lines, "uint8_t", mask_name, mask)
        definitions.append((symbol, pixel_name, mask_name))
    for symbol, pixels, mask in definitions:
        lines.append(
            f"const QingyunPetSprite kQingyunPet{symbol} = "
            f"{{{pixels}, {mask}}};"
        )
    lines.append("")
    PET_HEADER.write_text("\n".join(lines), encoding="utf-8")


def main() -> None:
    convert_scene()
    convert_icons()
    convert_pets()


if __name__ == "__main__":
    main()
