from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "assets/processed/ui/button_icons"
OUTPUT = ROOT / "firmware/ai_pet/assets/home_button_icons.h"
PREVIEW = SOURCE / "firmware_preview"
SIZE = 14
ICONS = {
    "Interact": "button-icon-1.png",
    "Care": "button-icon-2.png",
    "Adventure": "button-icon-3.png",
    "Status": "button-icon-4.png",
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
    width = max(1, round(subject.width * scale))
    height = max(1, round(subject.height * scale))
    subject = subject.resize((width, height), Image.Resampling.NEAREST)
    frame = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    frame.alpha_composite(
        subject, ((SIZE - width) // 2, (SIZE - height) // 2)
    )
    return frame


def append_array(
    lines: list[str], kind: str, name: str, values: list[int]
) -> None:
    width = 4 if kind == "uint16_t" else 2
    lines.append(f"const {kind} {name}[] PROGMEM = {{")
    for offset in range(0, len(values), 14):
        chunk = values[offset : offset + 14]
        lines.append(
            "  "
            + ", ".join(f"0x{value:0{width}X}" for value in chunk)
            + ","
        )
    lines.extend(("};", ""))


def main() -> None:
    PREVIEW.mkdir(parents=True, exist_ok=True)
    lines = [
        "#pragma once",
        "",
        "#include <Arduino.h>",
        "",
        f"constexpr uint8_t kHomeButtonIconWidth = {SIZE};",
        f"constexpr uint8_t kHomeButtonIconHeight = {SIZE};",
        "",
        "struct HomeButtonIcon {",
        "  const uint16_t* pixels;",
        "  const uint8_t* mask;",
        "};",
        "",
    ]
    definitions: list[tuple[str, str, str]] = []
    for label, filename in ICONS.items():
        frame = normalize(SOURCE / filename)
        frame.save(PREVIEW / f"{label.lower()}.png")
        pixels = [
            rgb565(red, green, blue)
            for red, green, blue, _ in frame.getdata()
        ]
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
        pixel_name = f"kHomeButton{label}Pixels"
        mask_name = f"kHomeButton{label}Mask"
        append_array(lines, "uint16_t", pixel_name, pixels)
        append_array(lines, "uint8_t", mask_name, mask)
        definitions.append((label, pixel_name, mask_name))

    for label, pixels, mask in definitions:
        lines.append(
            f"const HomeButtonIcon kHomeButton{label} = "
            f"{{{pixels}, {mask}}};"
        )
    lines.append("")
    OUTPUT.write_text("\n".join(lines), encoding="utf-8")


if __name__ == "__main__":
    main()
