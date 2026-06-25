from __future__ import annotations

from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
BOSS_SOURCE = ROOT / "assets/processed/creatures/boss_a1"
BOSS_HEADER = ROOT / "firmware/ai_pet/assets/qingyun_boss.h"
BOSS_SIZE = 48
BOSS_LARGE_SIZE = 72
BOSS_FRAMES = 4
ALPHA_THRESHOLD = 96


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


def main() -> None:
    lines = [
        "#pragma once",
        "",
        "#include <Arduino.h>",
        "",
        f"constexpr uint8_t kQingyunBossWidth = {BOSS_SIZE};",
        f"constexpr uint8_t kQingyunBossHeight = {BOSS_SIZE};",
        f"constexpr uint8_t kQingyunBossFrameCount = {BOSS_FRAMES};",
        "",
    ]

    all_pixels: list[list[int]] = []
    all_masks: list[list[int]] = []

    for frame_index in range(BOSS_FRAMES):
        source = Image.open(
            BOSS_SOURCE / f"idle-{frame_index + 1}.png"
        ).convert("RGBA")

        bbox = source.getchannel("A").getbbox()
        if bbox is None:
            raise ValueError(f"empty boss frame: {frame_index + 1}")
        subject = source.crop(bbox)

        scale = min(
            (BOSS_SIZE - 2) / subject.width,
            (BOSS_SIZE - 2) / subject.height,
        )
        new_w = max(1, round(subject.width * scale))
        new_h = max(1, round(subject.height * scale))
        subject = subject.resize(
            (new_w, new_h), Image.Resampling.NEAREST
        )

        canvas = Image.new("RGBA", (BOSS_SIZE, BOSS_SIZE), (0, 0, 0, 0))
        canvas.alpha_composite(
            subject,
            ((BOSS_SIZE - new_w) // 2, BOSS_SIZE - new_h - 1),
        )

        pixels = [
            rgb565(*canvas.getpixel((x, y))[:3])
            for y in range(BOSS_SIZE)
            for x in range(BOSS_SIZE)
        ]
        mask: list[int] = []
        for y in range(BOSS_SIZE):
            for byte_x in range(0, BOSS_SIZE, 8):
                value = 0
                for bit in range(8):
                    x = byte_x + bit
                    if canvas.getpixel((x, y))[3] >= ALPHA_THRESHOLD:
                        value |= 0x80 >> bit
                mask.append(value)

        all_pixels.append(pixels)
        all_masks.append(mask)

    for frame_index in range(BOSS_FRAMES):
        append_array(
            lines,
            "uint16_t",
            f"kQingyunBossFrame{frame_index}Pixels",
            all_pixels[frame_index],
        )
        append_array(
            lines,
            "uint8_t",
            f"kQingyunBossFrame{frame_index}Mask",
            all_masks[frame_index],
        )

    lines.append(
        "const uint16_t* const kQingyunBossPixels[] PROGMEM = {"
    )
    for i in range(BOSS_FRAMES):
        lines.append(f"  kQingyunBossFrame{i}Pixels,")
    lines.append("};")
    lines.append("")

    lines.append(
        "const uint8_t* const kQingyunBossMasks[] PROGMEM = {"
    )
    for i in range(BOSS_FRAMES):
        lines.append(f"  kQingyunBossFrame{i}Mask,")
    lines.append("};")
    lines.append("")

    # Large boss sprite for prompt page (single frame)
    source = Image.open(BOSS_SOURCE / "idle-1.png").convert("RGBA")
    bbox = source.getchannel("A").getbbox()
    if bbox is None:
        raise ValueError("empty boss frame for large sprite")
    subject = source.crop(bbox)
    scale = min(
        (BOSS_LARGE_SIZE - 2) / subject.width,
        (BOSS_LARGE_SIZE - 2) / subject.height,
    )
    new_w = max(1, round(subject.width * scale))
    new_h = max(1, round(subject.height * scale))
    subject = subject.resize((new_w, new_h), Image.Resampling.NEAREST)
    large_canvas = Image.new(
        "RGBA", (BOSS_LARGE_SIZE, BOSS_LARGE_SIZE), (0, 0, 0, 0)
    )
    large_canvas.alpha_composite(
        subject,
        ((BOSS_LARGE_SIZE - new_w) // 2, BOSS_LARGE_SIZE - new_h - 1),
    )

    large_pixels = [
        rgb565(*large_canvas.getpixel((x, y))[:3])
        for y in range(BOSS_LARGE_SIZE)
        for x in range(BOSS_LARGE_SIZE)
    ]
    large_mask: list[int] = []
    for y in range(BOSS_LARGE_SIZE):
        for byte_x in range(0, BOSS_LARGE_SIZE, 8):
            value = 0
            for bit in range(8):
                px = byte_x + bit
                if large_canvas.getpixel((px, y))[3] >= ALPHA_THRESHOLD:
                    value |= 0x80 >> bit
            large_mask.append(value)

    lines.extend((
        f"constexpr uint8_t kQingyunBossLargeWidth = {BOSS_LARGE_SIZE};",
        f"constexpr uint8_t kQingyunBossLargeHeight = {BOSS_LARGE_SIZE};",
        "",
    ))
    append_array(lines, "uint16_t",
                 "kQingyunBossLargePixels", large_pixels)
    append_array(lines, "uint8_t",
                 "kQingyunBossLargeMask", large_mask)

    BOSS_HEADER.write_text("\n".join(lines), encoding="utf-8")
    print(f"Boss header written: {BOSS_HEADER}")
    print(f"  {BOSS_FRAMES} frames, {BOSS_SIZE}x{BOSS_SIZE} each")
    print(f"  + 1 large frame {BOSS_LARGE_SIZE}x{BOSS_LARGE_SIZE}")
    total_bytes = (
        BOSS_FRAMES * (BOSS_SIZE**2 * 2 + BOSS_SIZE**2 // 8)
        + (BOSS_LARGE_SIZE**2 * 2 + BOSS_LARGE_SIZE**2 // 8)
    )
    print(f"  PROGMEM usage: ~{total_bytes} bytes ({total_bytes / 1024:.1f} KB)")


if __name__ == "__main__":
    main()
