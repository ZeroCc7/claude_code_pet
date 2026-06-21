from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "assets/processed/ui/home_icons"
OUTPUT = ROOT / "firmware/ai_pet/assets/home_ui_icons.h"
PREVIEW = ROOT / "assets/processed/ui/home_icons/firmware_preview"
SIZE = 14
ICONS = {
    "Lotus": "ui-icons-1.png",
    "Heart": "ui-icons-2.png",
    "Energy": "ui-icons-3.png",
    "Crystal": "ui-icons-4.png",
}


def rgb565(red: int, green: int, blue: int) -> int:
    return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3)


def normalize(path: Path) -> Image.Image:
    image = Image.open(path).convert("RGBA")
    bbox = image.getchannel("A").getbbox()
    if bbox is None:
        raise ValueError(f"empty icon: {path}")
    subject = image.crop(bbox)
    scale = min(13 / subject.width, 13 / subject.height)
    size = (max(1, round(subject.width * scale)),
            max(1, round(subject.height * scale)))
    subject = subject.resize(size, Image.Resampling.NEAREST)
    frame = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    frame.alpha_composite(subject, ((SIZE - size[0]) // 2,
                                    (SIZE - size[1]) // 2))
    return frame


def append_array(lines: list[str], kind: str, name: str,
                 values: list[int]) -> None:
    width = 4 if kind == "uint16_t" else 2
    per_line = 14
    lines.append(f"const {kind} {name}[] PROGMEM = {{")
    for offset in range(0, len(values), per_line):
        chunk = values[offset:offset + per_line]
        lines.append("  " + ", ".join(
            f"0x{value:0{width}X}" for value in chunk) + ",")
    lines.append("};")
    lines.append("")


def main() -> None:
    PREVIEW.mkdir(parents=True, exist_ok=True)
    lines = [
        "#pragma once",
        "",
        "#include <Arduino.h>",
        "",
        f"constexpr uint8_t kHomeIconWidth = {SIZE};",
        f"constexpr uint8_t kHomeIconHeight = {SIZE};",
        "",
        "struct HomeUiIcon {",
        "  const uint16_t* pixels;",
        "  const uint8_t* mask;",
        "};",
        "",
    ]
    definitions: list[tuple[str, str, str]] = []
    for label, filename in ICONS.items():
        frame = normalize(SOURCE / filename)
        frame.save(PREVIEW / f"{label.lower()}.png")
        pixels = [rgb565(red, green, blue)
                  for red, green, blue, _ in frame.getdata()]
        mask: list[int] = []
        for y in range(SIZE):
            for byte_x in range(0, SIZE, 8):
                value = 0
                for bit in range(8):
                    x = byte_x + bit
                    alpha = frame.getpixel((x, y))[3] if x < SIZE else 0
                    if alpha >= 96:
                        value |= 0x80 >> bit
                mask.append(value)
        pixel_name = f"kHomeIcon{label}Pixels"
        mask_name = f"kHomeIcon{label}Mask"
        append_array(lines, "uint16_t", pixel_name, pixels)
        append_array(lines, "uint8_t", mask_name, mask)
        definitions.append((label, pixel_name, mask_name))

    for label, pixels, mask in definitions:
        lines.append(
            f"const HomeUiIcon kHomeIcon{label} = {{{pixels}, {mask}}};")
    lines.append("")
    OUTPUT.write_text("\n".join(lines), encoding="utf-8")


if __name__ == "__main__":
    main()
