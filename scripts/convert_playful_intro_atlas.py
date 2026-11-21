#!/usr/bin/env python3
from argparse import ArgumentParser
import math
from pathlib import Path
from typing import List

from PIL import Image


def load_frames(path: Path) -> List[Image.Image]:
    gif = Image.open(path)
    frames: List[Image.Image] = []
    for i in range(gif.n_frames):
        gif.seek(i)
        frames.append(gif.convert("RGBA"))
    return frames


def load_frames_from_atlas(path: Path, columns: int, frames: int) -> List[Image.Image]:
    atlas = Image.open(path).convert("RGBA")
    cols = max(1, columns)
    total_frames = max(1, frames)
    rows = math.ceil(total_frames / cols)
    frame_w = atlas.width // cols
    frame_h = atlas.height // rows
    out: List[Image.Image] = []
    for idx in range(total_frames):
        col = idx % cols
        row = idx // cols
        box = (
            col * frame_w,
            row * frame_h,
            (col + 1) * frame_w,
            (row + 1) * frame_h,
        )
        out.append(atlas.crop(box))
    return out


def add_step_inbetweens(frames: List[Image.Image], steps: int = 2) -> List[Image.Image]:
    """
    Insert intermediate frames between consecutive frames by progressively
    applying changed pixels. steps=2 yields two in-betweens that apply roughly
    1/3 and 2/3 of the changed pixels.
    """
    if steps <= 0 or len(frames) < 2:
        return frames

    result: List[Image.Image] = []
    for idx in range(len(frames) - 1):
        a = frames[idx].convert("RGBA")
        b = frames[idx + 1].convert("RGBA")
        aw, ah = a.size
        a_data = list(a.getdata())
        b_data = list(b.getdata())
        diff_indices = [i for i, (pa, pb) in enumerate(zip(a_data, b_data)) if pa != pb]
        total = len(diff_indices)
        if total == 0:
            # no difference; just duplicate
            result.append(a)
            continue

        one_third = math.ceil(total / 3.0)
        two_third = math.ceil(total * 2.0 / 3.0)

        def make_frame(count: int) -> Image.Image:
            data = list(a_data)
            for i in diff_indices[:count]:
                data[i] = b_data[i]
            img = Image.new("RGBA", (aw, ah))
            img.putdata(data)
            return img

        result.append(a)
        result.append(make_frame(one_third))
        if steps >= 2:
            result.append(make_frame(two_third))
        result.append(b)

    result.append(frames[-1])
    return result


def recolor_frame(
    frame: Image.Image,
    outline_color,
    stem_color,
    text_color,
    bg_color,
    outline_thresh: int,
    text_thresh: int,
    stem_x_max_ratio: float,
    stem_y_min_ratio: float,
    stem_y_max_ratio: float,
    remove_text: bool,
    clear_box: bool,
    clear_box_x_ratio: float,
    clear_box_y_ratio: float,
) -> Image.Image:
    gray = frame.convert("L")
    w, h = frame.size
    x_cut = int(w * stem_x_max_ratio)
    y_min = int(h * stem_y_min_ratio)
    y_max = int(h * stem_y_max_ratio)

    pixels = list(gray.getdata())
    new_pixels = []
    for idx, v in enumerate(pixels):
        y, x = divmod(idx, w)
        is_stem_window = x < x_cut and y_min <= y <= y_max
        if v <= outline_thresh:
            if is_stem_window:
                new_pixels.append(stem_color)
            else:
                new_pixels.append(outline_color)
        elif v <= text_thresh:
            new_pixels.append(bg_color if remove_text else text_color)
        else:
            new_pixels.append(bg_color)
    out = Image.new("RGBA", frame.size)
    out.putdata(new_pixels)

    if clear_box:
        x0 = int(w * clear_box_x_ratio)
        y0 = int(h * clear_box_y_ratio)
        for y in range(y0, h):
            for x in range(x0, w):
                out.putpixel((x, y), bg_color)

    return out


def make_atlas(frames: List[Image.Image], bg_color, columns: int) -> Image.Image:
    if not frames:
        raise ValueError("no frames to build atlas")
    w, h = frames[0].size
    cols = max(1, columns)
    rows = math.ceil(len(frames) / cols)
    atlas = Image.new("RGBA", (w * cols, h * rows), bg_color)
    for idx, fr in enumerate(frames):
        col = idx % cols
        row = idx // cols
        atlas.paste(fr, (col * w, row * h))
    return atlas


def main():
    parser = ArgumentParser()
    parser.add_argument("--input", default="resources/playful.gif")
    parser.add_argument("--input-atlas", default="", help="Slice existing atlas instead of GIF")
    parser.add_argument("--output", default="resources/images/playful_intro_atlas.png")
    parser.add_argument("--outline-threshold", type=int, default=80)
    parser.add_argument("--text-threshold", type=int, default=170)
    parser.add_argument("--stem-x-max-ratio", type=float, default=0.2)
    parser.add_argument("--stem-y-min-ratio", type=float, default=0.25)
    parser.add_argument("--stem-y-max-ratio", type=float, default=0.78)
    parser.add_argument("--remove-text", action="store_true", help="Map text to background instead of white")
    parser.add_argument("--no-clear-box", action="store_true", help="Disable clearing bottom-right watermark box")
    parser.add_argument("--clear-box-x-ratio", type=float, default=0.8, help="Start X ratio for clearing box")
    parser.add_argument("--clear-box-y-ratio", type=float, default=0.8, help="Start Y ratio for clearing box")
    parser.add_argument("--columns", type=int, default=5, help="Atlas columns to keep texture under GPU limits")
    parser.add_argument("--frames", type=int, default=19, help="Frame count when slicing an existing atlas")
    parser.add_argument("--augment-only", action="store_true", help="Skip recolor and only add in-between frames")
    args = parser.parse_args()

    outline_color = (255, 140, 0, 255)
    stem_color = (60, 170, 90, 255)
    text_color = (255, 240, 220, 255)
    bg_color = (0, 0, 0, 255)

    if args.input_atlas:
        frames = load_frames_from_atlas(Path(args.input_atlas), args.columns, args.frames)
    else:
        frames = load_frames(Path(args.input))

    if args.augment_only:
        processed = add_step_inbetweens(frames, steps=2)
    else:
        recolored = [
            recolor_frame(
                fr,
                outline_color,
                stem_color,
                text_color,
                bg_color,
                args.outline_threshold,
                args.text_threshold,
                args.stem_x_max_ratio,
                args.stem_y_min_ratio,
                args.stem_y_max_ratio,
                args.remove_text,
                not args.no_clear_box,
                args.clear_box_x_ratio,
                args.clear_box_y_ratio,
            )
            for fr in frames
        ]
        processed = add_step_inbetweens(recolored, steps=2)

    atlas = make_atlas(processed, bg_color, args.columns)
    out_path = Path(args.output)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    atlas.save(out_path)
    print(f"wrote {out_path} with {len(processed)} frames size {atlas.size}")


if __name__ == "__main__":
    main()
