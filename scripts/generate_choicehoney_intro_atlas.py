#!/usr/bin/env python3
"""
Generate the choicehoney intro sprite atlas.
- Produces resources/images/choicehoney_intro_atlas.png
- 12 frames, 512x256 each, arranged horizontally.
- Pepper outline + write-on cursive-style text.
Requires: Pillow (pip install Pillow)
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont, ImageChops

FRAMES = 12
# Frame enlarged further
FRAME_W, FRAME_H = 960, 390
BG = (0, 0, 0, 0)
OUTLINE_COLOR = (255, 140, 0, 255)
STEM_COLOR = (60, 170, 90, 255)
TEXT_COLOR = (255, 240, 220, 255)
PEPPER_THICKNESS = 12
TEXT = "choicehoney"

FONT_CANDIDATES = [
    "/System/Library/Fonts/Supplemental/Apple Chancery.ttf",
    "resources/fonts/Gaegu-Bold.ttf",
    "resources/fonts/Gaegu-Regular.ttf",
    "resources/fonts/constan.ttf",
]

# Base pepper outline (rough bezier-ish polyline), scaled later. First == last.
BASE_OUTLINE = [
    (40, 90), (70, 60), (130, 50), (200, 65), (250, 100), (240, 130),
    (190, 145), (120, 140), (70, 120), (50, 105), (40, 90),
]


def pick_font():
    for path in FONT_CANDIDATES:
        p = Path(path)
        if p.exists():
            try:
                return ImageFont.truetype(str(p), 150)
            except Exception:
                continue
    return ImageFont.load_default()


def scale_outline(outline, target_w, target_h, scale):
    scaled = [(x * scale, y * scale) for x, y in outline]
    minx = min(p[0] for p in scaled)
    maxx = max(p[0] for p in scaled)
    miny = min(p[1] for p in scaled)
    maxy = max(p[1] for p in scaled)
    w = maxx - minx
    h = maxy - miny
    off_x = (target_w - w) / 2 - minx
    off_y = (target_h - h) / 2 - miny
    return [(x + off_x, y + off_y) for x, y in scaled]


def catmull_rom(points, samples_per_segment=12, closed=True):
    """Return a smoothed point list using Catmull-Rom splines."""
    pts = list(points)
    if closed:
        pts = [pts[-1]] + pts + pts[:2]
    else:
        pts = [pts[0]] + pts + [pts[-1], pts[-1]]

    out = []
    for i in range(len(pts) - 3):
        p0, p1, p2, p3 = pts[i], pts[i + 1], pts[i + 2], pts[i + 3]
        for j in range(samples_per_segment):
            t = j / float(samples_per_segment)
            t2 = t * t
            t3 = t2 * t
            x = 0.5 * (
                (2 * p1[0])
                + (-p0[0] + p2[0]) * t
                + (2 * p0[0] - 5 * p1[0] + 4 * p2[0] - p3[0]) * t2
                + (-p0[0] + 3 * p1[0] - 3 * p2[0] + p3[0]) * t3
            )
            y = 0.5 * (
                (2 * p1[1])
                + (-p0[1] + p2[1]) * t
                + (2 * p0[1] - 5 * p1[1] + 4 * p2[1] - p3[1]) * t2
                + (-p0[1] + 3 * p1[1] - 3 * p2[1] + p3[1]) * t3
            )
            out.append((x, y))
    if closed:
        out.append(out[0])
    return out


def build_frames(font, outline):
    text_img = Image.new("RGBA", (FRAME_W, FRAME_H), BG)
    text_draw = ImageDraw.Draw(text_img)
    text_bbox = text_draw.textbbox((0, 0), TEXT, font=font)
    text_w = text_bbox[2] - text_bbox[0]
    text_h = text_bbox[3] - text_bbox[1]
    text_x = (FRAME_W - text_w) // 2
    text_y = (FRAME_H - text_h) // 2 - 10  # bump text up slightly (more lift)

    full_text = Image.new("RGBA", (FRAME_W, FRAME_H), BG)
    ImageDraw.Draw(full_text).text((text_x, text_y), TEXT, font=font, fill=TEXT_COLOR)
    full_text_alpha = full_text.split()[-1]

    text_angle = -5.0  # slight pitch (more tilt)
    frames = []
    for i in range(FRAMES):
        frac = (i + 1) / FRAMES
        frame = Image.new("RGBA", (FRAME_W, FRAME_H), BG)
        d = ImageDraw.Draw(frame)
        # Stem first (behind text)
        minx = min(p[0] for p in outline)
        miny = min(p[1] for p in outline)
        maxy = max(p[1] for p in outline)
        midy = (miny + maxy) / 2
        stem_len = 240  # ~20% longer
        stem_start = (minx - 90, midy + 12)
        stem_mid = (stem_start[0] + stem_len * 0.5, midy - 35)
        stem_end = (stem_start[0] + stem_len, midy + 10)
        stem_path = [stem_start, stem_mid, stem_end]
        # Clip stem so it only shows outside the pepper (approx by y above mid and x < minx)
        clipped_stem = []
        for pt in stem_path:
            x, y = pt
            if x < minx and y <= midy + 10:
                clipped_stem.append(pt)
        # If clipping removed points, fall back to original
        if len(clipped_stem) >= 2:
            d.line(clipped_stem, fill=STEM_COLOR, width=PEPPER_THICKNESS * 5)
        else:
            d.line(stem_path, fill=STEM_COLOR, width=PEPPER_THICKNESS * 5)

        # Text next
        mask = Image.new("L", (FRAME_W, FRAME_H), 0)
        mx = int(text_x + text_w * frac)
        ImageDraw.Draw(mask).rectangle([text_x, text_y, mx, text_y + text_h], fill=255)
        # Keep only glyph alpha, no solid rect background
        combined_alpha = ImageChops.multiply(full_text_alpha, mask)
        text_layer = full_text.copy()
        text_layer.putalpha(combined_alpha)
        text_rot = text_layer.rotate(
            text_angle, resample=Image.BICUBIC, expand=True, fillcolor=BG
        )
        px = (FRAME_W - text_rot.width) // 2
        py = (FRAME_H - text_rot.height) // 2
        frame.paste(text_rot, (px, py), text_rot)

        # Pepper outline (explicitly closed) drawn last
        d.line(outline + [outline[0]], fill=OUTLINE_COLOR, width=PEPPER_THICKNESS, joint="curve")
        frames.append(frame)
    return frames


def save_atlas(frames, path):
    atlas_w = FRAME_W * len(frames)
    atlas_h = FRAME_H
    atlas = Image.new("RGBA", (atlas_w, atlas_h), BG)
    for idx, fr in enumerate(frames):
        atlas.paste(fr, (idx * FRAME_W, 0))
    path.parent.mkdir(parents=True, exist_ok=True)
    atlas.save(path)
    print(f"wrote {path} size {atlas.size}")


def main():
    font = pick_font()
    outline = scale_outline(BASE_OUTLINE, FRAME_W, FRAME_H, scale=3.6)
    outline = catmull_rom(outline, samples_per_segment=16, closed=True)
    frames = build_frames(font, outline)
    out_path = Path("resources/images/choicehoney_intro_atlas.png")
    save_atlas(frames, out_path)


if __name__ == "__main__":
    main()

