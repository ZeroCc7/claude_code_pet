from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
FRAME_SIZE = 62
FINAL_FRAME_SIZE = (64, 82)
FINAL_SUBJECT_SIZE = (62, 80)
FORMS = {
    "egg": ROOT / "assets/processed/pets/egg",
    "rookie_a": ROOT / "assets/processed/pets/rookie_a",
    "rookie_b": ROOT / "assets/processed/pets/rookie_b",
}
FINAL_FORMS = {
    "final_a1": ROOT / "assets/processed/pets/final_a1",
    "final_a2": ROOT / "assets/processed/pets/final_a2",
    "final_b1": ROOT / "assets/processed/pets/final_b1",
    "final_b2": ROOT / "assets/processed/pets/final_b2",
}
OUTPUT = ROOT / "firmware/ai_pet/assets/pet_sprites.h"


def rgb565(red: int, green: int, blue: int) -> int:
    return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3)


def normalize_frame(path: Path) -> Image.Image:
    source = Image.open(path).convert("RGBA")
    alpha = source.getchannel("A")
    bbox = alpha.getbbox()
    if bbox is None:
        raise ValueError(f"empty sprite frame: {path}")
    subject = source.crop(bbox)
    scale = min(60 / subject.width, 60 / subject.height)
    size = (
        max(1, round(subject.width * scale)),
        max(1, round(subject.height * scale)),
    )
    subject = subject.resize(size, Image.Resampling.NEAREST)
    frame = Image.new("RGBA", (FRAME_SIZE, FRAME_SIZE), (0, 0, 0, 0))
    x = (FRAME_SIZE - size[0]) // 2
    y = FRAME_SIZE - size[1] - 1
    frame.alpha_composite(subject, (x, y))
    return frame


def union_bbox(paths: list[Path]) -> tuple[int, int, int, int]:
    boxes = []
    for path in paths:
        bbox = Image.open(path).convert("RGBA").getchannel("A").getbbox()
        if bbox is None:
            raise ValueError(f"empty sprite frame: {path}")
        boxes.append(bbox)
    return (
        min(box[0] for box in boxes),
        min(box[1] for box in boxes),
        max(box[2] for box in boxes),
        max(box[3] for box in boxes),
    )


def normalize_final_frame(
    path: Path, crop_box: tuple[int, int, int, int]
) -> Image.Image:
    source = Image.open(path).convert("RGBA")
    subject = source.crop(crop_box)
    scale = min(
        FINAL_SUBJECT_SIZE[0] / subject.width,
        FINAL_SUBJECT_SIZE[1] / subject.height,
    )
    size = (
        max(1, round(subject.width * scale)),
        max(1, round(subject.height * scale)),
    )
    subject = subject.resize(size, Image.Resampling.NEAREST)
    frame = Image.new("RGBA", FINAL_FRAME_SIZE, (0, 0, 0, 0))
    x = (FINAL_FRAME_SIZE[0] - size[0]) // 2
    y = FINAL_FRAME_SIZE[1] - size[1]
    frame.alpha_composite(subject, (x, y))
    return frame


def encode_frame(frame: Image.Image) -> tuple[list[int], list[int]]:
    pixels = [rgb565(red, green, blue) for red, green, blue, _ in frame.getdata()]
    mask: list[int] = []
    for y in range(frame.height):
        for byte_x in range(0, frame.width, 8):
            value = 0
            for bit in range(8):
                x = byte_x + bit
                alpha_value = frame.getpixel((x, y))[3] if x < frame.width else 0
                if alpha_value >= 96:
                    value |= 0x80 >> bit
            mask.append(value)
    return pixels, mask


def write_array(lines: list[str], kind: str, name: str, values: list[int]) -> None:
    lines.append(f"const {kind} {name}[] PROGMEM = {{")
    per_line = 12 if kind == "uint16_t" else 16
    width = 4 if kind == "uint16_t" else 2
    for index in range(0, len(values), per_line):
        chunk = values[index : index + per_line]
        rendered = ", ".join(f"0x{value:0{width}X}" for value in chunk)
        lines.append(f"  {rendered},")
    lines.append("};")
    lines.append("")


def convert() -> None:
    lines = [
        "#pragma once",
        "",
        "#include <Arduino.h>",
        "",
        f"constexpr uint8_t kPetSpriteWidth = {FRAME_SIZE};",
        f"constexpr uint8_t kPetSpriteHeight = {FRAME_SIZE};",
        "constexpr uint8_t kPetSpriteFrameCount = 4;",
        f"constexpr uint8_t kFinalPetSpriteWidth = {FINAL_FRAME_SIZE[0]};",
        f"constexpr uint8_t kFinalPetSpriteHeight = {FINAL_FRAME_SIZE[1]};",
        "",
        "struct PetSpriteFrame {",
        "  const uint16_t* pixels;",
        "  const uint8_t* mask;",
        "};",
        "",
    ]
    frame_names: dict[str, list[tuple[str, str]]] = {}
    preview_root = ROOT / "assets/processed/pets/firmware_preview"
    preview_root.mkdir(parents=True, exist_ok=True)

    for form, directory in FORMS.items():
        frame_names[form] = []
        form_preview = preview_root / form
        form_preview.mkdir(parents=True, exist_ok=True)
        for frame_index in range(4):
            frame = normalize_frame(directory / f"idle-{frame_index + 1}.png")
            frame.save(form_preview / f"frame-{frame_index + 1}.png")
            pixels, mask = encode_frame(frame)
            pixel_name = f"kPet_{form}_{frame_index}_pixels"
            mask_name = f"kPet_{form}_{frame_index}_mask"
            write_array(lines, "uint16_t", pixel_name, pixels)
            write_array(lines, "uint8_t", mask_name, mask)
            frame_names[form].append((pixel_name, mask_name))

    for form, directory in FINAL_FORMS.items():
        frame_names[form] = []
        form_preview = preview_root / form
        form_preview.mkdir(parents=True, exist_ok=True)
        paths = [directory / f"idle-{index}.png" for index in range(1, 5)]
        crop_box = union_bbox(paths)
        for frame_index, path in enumerate(paths):
            frame = normalize_final_frame(path, crop_box)
            frame.save(form_preview / f"frame-{frame_index + 1}.png")
            pixels, mask = encode_frame(frame)
            pixel_name = f"kPet_{form}_{frame_index}_pixels"
            mask_name = f"kPet_{form}_{frame_index}_mask"
            write_array(lines, "uint16_t", pixel_name, pixels)
            write_array(lines, "uint8_t", mask_name, mask)
            frame_names[form].append((pixel_name, mask_name))

    for form, frames in frame_names.items():
        lines.append(f"const PetSpriteFrame kPet_{form}_frames[] = {{")
        for pixels, mask in frames:
            lines.append(f"  {{{pixels}, {mask}}},")
        lines.append("};")
        lines.append("")

    OUTPUT.write_text("\n".join(lines), encoding="utf-8")


if __name__ == "__main__":
    convert()
