from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image


def rgb565(red: int, green: int, blue: int) -> int:
    return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("input", type=Path)
    parser.add_argument("--preview", type=Path, required=True)
    parser.add_argument("--header", type=Path, required=True)
    parser.add_argument("--symbol", default="kImmortalCaveHome")
    parser.add_argument("--crop-y", type=int, default=270)
    args = parser.parse_args()

    source = Image.open(args.input).convert("RGB")
    crop_height = round(source.width / (128 / 160))
    crop_y = max(0, min(args.crop_y, source.height - crop_height))
    cropped = source.crop((0, crop_y, source.width, crop_y + crop_height))
    resized = cropped.resize((128, 160), Image.Resampling.LANCZOS)
    quantized = resized.quantize(colors=64, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    output = quantized.convert("RGB")

    args.preview.parent.mkdir(parents=True, exist_ok=True)
    args.header.parent.mkdir(parents=True, exist_ok=True)
    output.save(args.preview)

    pixels = [rgb565(*pixel) for pixel in output.getdata()]
    lines = []
    for offset in range(0, len(pixels), 12):
        chunk = ", ".join(f"0x{value:04X}" for value in pixels[offset : offset + 12])
        lines.append(f"  {chunk},")

    args.header.write_text(
        "#pragma once\n\n"
        "#include <Arduino.h>\n\n"
        f"constexpr uint16_t {args.symbol}Width = 128;\n"
        f"constexpr uint16_t {args.symbol}Height = 160;\n"
        f"const uint16_t {args.symbol}[] PROGMEM = {{\n"
        + "\n".join(lines)
        + "\n};\n",
        encoding="utf-8",
    )


if __name__ == "__main__":
    main()
