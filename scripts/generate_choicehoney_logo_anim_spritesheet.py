#!/usr/bin/env python3
"""
ChoiceHoney logo (jalapeño) + reveal animation spritesheet.

Outputs:
- resources/ui/choicehoney_logo_final.svg (transparent background)
- resources/ui/choicehoney_logo_final.png (transparent background)
- resources/images/choicehoney_logo_anim_spritesheet.png (black background)
- resources/images/choicehoney_logo_anim_spritesheet.json (frame metadata)

Requires: Pillow (pip install Pillow)
"""

from __future__ import annotations

import json
import math
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, List, Sequence, Tuple

from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageFont


@dataclass(frozen=True)
class Color:
    r: int
    g: int
    b: int
    a: int = 255

    def rgba(self) -> Tuple[int, int, int, int]:
        return (self.r, self.g, self.b, self.a)


# --- User-facing look constants ------------------------------------------------
BG_BLACK = Color(0, 0, 0, 255)
WHITE = Color(255, 255, 255, 255)

# Vibrant jalapeño green + darker stem
PEPPER_GREEN = Color(34, 197, 94, 255)  # vibrant green
STEM_DARK = Color(32, 58, 35, 255)  # dark forest-ish green

# --- Output / layout -----------------------------------------------------------
FRAME_W, FRAME_H = 1024, 512
FPS = 30

# Frame count and segmenting for the 5-step sequence
FRAMES_SOLID = 10
FRAMES_KNIFE = 10
FRAMES_FALL = 28
FRAMES_DISSOLVE = 6
TOTAL_FRAMES = FRAMES_SOLID + FRAMES_KNIFE + FRAMES_FALL + FRAMES_DISSOLVE

# Spritesheet packing (avoid a super-wide atlas)
SHEET_COLS = 8

TEXT = "CHOICEHONEY"

FONT_CANDIDATES = [
    "resources/fonts/NotoSansKR.ttf",
    "resources/fonts/Gaegu-Bold.ttf",
    "resources/fonts/constan.ttf",
]


def clamp(x: float, lo: float, hi: float) -> float:
    return max(lo, min(hi, x))


def lerp(a: float, b: float, t: float) -> float:
    return a + (b - a) * t


def ease_in_quad(t: float) -> float:
    t = clamp(t, 0.0, 1.0)
    return t * t


def ease_out_quad(t: float) -> float:
    t = clamp(t, 0.0, 1.0)
    return 1.0 - (1.0 - t) * (1.0 - t)


def piecewise_tilt_weighted(t: float) -> float:
    """
    Combine Ease-In (initial tilt) and Ease-Out (fall) in one curve.
    """
    t = clamp(t, 0.0, 1.0)
    split = 0.35
    if t <= split:
        return ease_in_quad(t / split) * split
    return split + ease_out_quad((t - split) / (1.0 - split)) * (1.0 - split)


def pick_font(size_px: int) -> ImageFont.FreeTypeFont | ImageFont.ImageFont:
    for path in FONT_CANDIDATES:
        p = Path(path)
        if p.exists():
            try:
                return ImageFont.truetype(str(p), size_px)
            except Exception:
                continue
    return ImageFont.load_default()


def catmull_rom(points: Sequence[Tuple[float, float]], samples_per_segment: int = 16) -> List[Tuple[float, float]]:
    """Closed Catmull-Rom spline sampling."""
    pts = list(points)
    # closed: pad ends
    pts = [pts[-1]] + pts + pts[:2]
    out: List[Tuple[float, float]] = []
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
    out.append(out[0])
    return out


def pepper_outline_points() -> List[Tuple[float, float]]:
    """
    Hand-tuned base outline points for a stylized jalapeño silhouette.
    This is a simple poly that we smooth with Catmull-Rom.
    """
    # Rough pepper body in local coordinates (0..1), slightly curved.
    base = [
        (0.10, 0.50),
        (0.14, 0.33),
        (0.26, 0.22),
        (0.44, 0.20),
        (0.64, 0.26),
        (0.80, 0.36),
        (0.90, 0.48),
        (0.88, 0.62),
        (0.78, 0.74),
        (0.60, 0.80),
        (0.40, 0.78),
        (0.22, 0.70),
        (0.14, 0.60),
        (0.10, 0.50),
    ]

    # Scale into frame, leaving margins for stem on the left.
    pad_x = int(FRAME_W * 0.12)
    pad_y = int(FRAME_H * 0.18)
    w = FRAME_W - pad_x * 2
    h = FRAME_H - pad_y * 2
    pts = [(pad_x + x * w, pad_y + y * h) for x, y in base]
    return catmull_rom(pts, samples_per_segment=18)


def points_bounds(pts: Iterable[Tuple[float, float]]) -> Tuple[float, float, float, float]:
    xs = [p[0] for p in pts]
    ys = [p[1] for p in pts]
    return min(xs), min(ys), max(xs), max(ys)


def draw_pepper_mask(size: Tuple[int, int], outline: Sequence[Tuple[float, float]]) -> Image.Image:
    mask = Image.new("L", size, 0)
    d = ImageDraw.Draw(mask)
    d.polygon(outline, fill=255)
    return mask


def make_inner_mask(pepper_mask: Image.Image, shrink_px: int) -> Image.Image:
    # Erode to create an "inner cavity" area.
    # MinFilter acts like erosion for white regions on L-mode.
    k = max(3, int(shrink_px) | 1)  # odd
    inner = pepper_mask.filter(ImageFilter.MinFilter(size=k))
    return inner


def make_text_layer(size: Tuple[int, int], inner_mask: Image.Image, outline: Sequence[Tuple[float, float]]) -> Image.Image:
    """
    Create white CHOICEHONEY text clipped to the inner cavity, with a mild warp so it
    feels like it follows the pepper's interior.
    """
    w, h = size
    layer = Image.new("RGBA", size, (0, 0, 0, 0))
    d = ImageDraw.Draw(layer)

    # Measure inner bounds to scale text to fill cavity edge-to-edge.
    bbox = inner_mask.getbbox() or (0, 0, w, h)
    ix0, iy0, ix1, iy1 = bbox
    inner_w = max(1, ix1 - ix0)
    inner_h = max(1, iy1 - iy0)

    # Big font size, then downscale to fit.
    font = pick_font(size_px=240)
    tb = d.textbbox((0, 0), TEXT, font=font)
    tw = tb[2] - tb[0]
    th = tb[3] - tb[1]

    # Fit inside inner cavity with small padding.
    pad = int(min(inner_w, inner_h) * 0.06)
    target_w = max(1, inner_w - pad * 2)
    target_h = max(1, inner_h - pad * 2)
    scale = min(target_w / max(1, tw), target_h / max(1, th)) * 1.06

    # Render text in a separate image, then scale and "bolden" by overdraw.
    text_img = Image.new("RGBA", (max(1, int(tw * scale) + 40), max(1, int(th * scale) + 40)), (0, 0, 0, 0))
    td = ImageDraw.Draw(text_img)
    # pseudo-bold: draw with a small offset grid
    ox, oy = 20, 18
    for dx, dy in [(0, 0), (1, 0), (0, 1), (1, 1), (2, 0)]:
        td.text((ox + dx, oy + dy), TEXT, font=font, fill=WHITE.rgba())
    text_img = text_img.resize((int(text_img.width * scale), int(text_img.height * scale)), resample=Image.Resampling.LANCZOS)

    # Mild curvature warp (vertical displacement varies along x).
    amp = inner_h * 0.035
    period = inner_w * 1.4
    warped = Image.new("RGBA", (inner_w, inner_h), (0, 0, 0, 0))
    # Center inside inner rect.
    px = (inner_w - text_img.width) // 2
    py = (inner_h - text_img.height) // 2
    tmp = Image.new("RGBA", (inner_w, inner_h), (0, 0, 0, 0))
    tmp.paste(text_img, (px, py), text_img)
    tmp_data = tmp.load()
    warped_data = warped.load()
    for x in range(inner_w):
        dy = int(math.sin((x / max(1.0, period)) * (2.0 * math.pi)) * amp)
        for y in range(inner_h):
            sy = y - dy
            if 0 <= sy < inner_h:
                warped_data[x, y] = tmp_data[x, sy]

    # Paste warped text into full-size layer.
    layer.paste(warped, (ix0, iy0), warped)

    # Clip to inner cavity mask.
    alpha = layer.split()[-1]
    clipped_alpha = ImageChops.multiply(alpha, inner_mask)
    layer.putalpha(clipped_alpha)
    return layer


def draw_stem(img: Image.Image, outline: Sequence[Tuple[float, float]]) -> None:
    d = ImageDraw.Draw(img)
    minx, miny, maxx, maxy = points_bounds(outline)
    midy = (miny + maxy) * 0.5

    # Simple tapered stem: a thick polyline plus a small knob.
    stem_len = (maxx - minx) * 0.18
    x0 = minx - stem_len * 0.60
    x1 = minx + stem_len * 0.10
    y0 = midy - (maxy - miny) * 0.10
    y1 = midy + (maxy - miny) * 0.06
    width = int((maxy - miny) * 0.18)

    d.line([(x0, y0), (x1, y1)], fill=STEM_DARK.rgba(), width=width)
    d.ellipse([x0 - width * 0.35, y0 - width * 0.35, x0 + width * 0.35, y0 + width * 0.35], fill=STEM_DARK.rgba())


def draw_pepper_solid(size: Tuple[int, int], outline: Sequence[Tuple[float, float]], bg: Color | None) -> Image.Image:
    if bg is None:
        img = Image.new("RGBA", size, (0, 0, 0, 0))
    else:
        img = Image.new("RGBA", size, bg.rgba())
    draw_stem(img, outline)
    d = ImageDraw.Draw(img)
    d.polygon(outline, fill=PEPPER_GREEN.rgba())
    return img


def draw_knife_line(img: Image.Image, outline: Sequence[Tuple[float, float]], t: float) -> None:
    """
    Minimalist white knife outline that enters from the left and slices horizontally.
    t in [0,1]
    """
    d = ImageDraw.Draw(img)
    minx, miny, maxx, maxy = points_bounds(outline)
    y = (miny + maxy) * 0.5
    # knife travels from offscreen left to past the pepper
    travel = (maxx - minx) * 1.25
    x = lerp(minx - travel, maxx + travel * 0.2, t)

    blade_len = (maxx - minx) * 0.30
    handle_len = blade_len * 0.45
    blade_h = (maxy - miny) * 0.08
    stroke = max(2, int((maxy - miny) * 0.012))

    # Blade as a thin outline rectangle with a point
    bx0 = x
    bx1 = x + blade_len
    by0 = y - blade_h * 0.4
    by1 = y + blade_h * 0.4

    # Outline: handle + blade + pointed tip
    hx0 = bx0 - handle_len
    hx1 = bx0
    hy0 = y - blade_h * 0.55
    hy1 = y + blade_h * 0.55

    # handle
    d.rounded_rectangle([hx0, hy0, hx1, hy1], radius=blade_h * 0.35, outline=WHITE.rgba(), width=stroke)
    # guard (small line)
    d.line([(bx0, y - blade_h * 0.65), (bx0, y + blade_h * 0.65)], fill=WHITE.rgba(), width=stroke)
    # blade body
    d.rectangle([bx0, by0, bx1, by1], outline=WHITE.rgba(), width=stroke)
    # pointed tip
    d.line([(bx1, by0), (bx1 + blade_h * 0.75, y)], fill=WHITE.rgba(), width=stroke)
    d.line([(bx1 + blade_h * 0.75, y), (bx1, by1)], fill=WHITE.rgba(), width=stroke)

    # slice line across pepper (only visible while cutting)
    d.line([(minx + 12, y), (maxx - 12, y)], fill=WHITE.rgba(), width=max(2, stroke))


def make_cover_slice(size: Tuple[int, int], pepper_mask: Image.Image, outline: Sequence[Tuple[float, float]]) -> Tuple[Image.Image, int]:
    """
    The 'front layer' that hides the inner text initially.
    We'll model it as the upper region of the pepper (above the horizontal cut).
    Returns (cover_rgba, hinge_y).
    """
    w, h = size
    minx, miny, maxx, maxy = points_bounds(outline)
    hinge_y = int((miny + maxy) * 0.52)

    cover = Image.new("RGBA", size, (0, 0, 0, 0))
    # green fill, clipped to pepper and to upper half region
    region = Image.new("L", size, 0)
    ImageDraw.Draw(region).rectangle([0, 0, w, hinge_y], fill=255)
    cover_mask = ImageChops.multiply(pepper_mask, region)
    cover_draw = ImageDraw.Draw(cover)
    cover_draw.polygon(outline, fill=PEPPER_GREEN.rgba())
    cover.putalpha(cover_mask)
    return cover, hinge_y


def transform_domino_fall(slice_img: Image.Image, hinge_y: int, t: float) -> Image.Image:
    """
    Fake 2.5D rotation around the hinge (bottom axis) using a perspective-like quad warp.
    t in [0,1], where 0 = upright (covering pepper), 1 = fully fallen forward.
    """
    w, h = slice_img.size
    tt = piecewise_tilt_weighted(t)

    # Only transform the visible area (everything), but keep hinge as anchor.
    # Define source quad as full image bounds.
    src = (0, 0, w, h)

    # Destination quad:
    # Bottom edge stays near hinge, top edge moves down and widens slightly.
    # This produces a "domino toward camera" feel.
    max_drop = int((h - hinge_y) * 0.10 + h * 0.20)
    drop = int(max_drop * tt)
    widen = int((w * 0.07) * tt)
    skew = int((w * 0.03) * tt)

    # We also compress height above hinge to simulate foreshortening.
    # top_y approaches hinge_y + drop.
    top_y = int(lerp(0, hinge_y + drop, tt))
    bottom_y = int(lerp(h, hinge_y + int(h * 0.22) + drop, tt))

    # Limit so we don't invert
    top_y = min(top_y, bottom_y - 2)

    dst_quad = (
        0 - widen + skew,
        top_y,
        w + widen + skew,
        top_y,
        w + int(widen * 0.55),
        bottom_y,
        0 - int(widen * 0.55),
        bottom_y,
    )

    # PIL QUAD expects 8-tuple mapping from output -> input (inverse map),
    # but passing dst_quad with Image.QUAD uses it as the source quad in the
    # input image for mapping to a rectangle. To keep things simple, we use
    # MESH by defining a single rect mapping.
    rect = (0, 0, w, h)
    mesh = [(rect, dst_quad)]
    out = slice_img.transform((w, h), Image.Transform.MESH, mesh, resample=Image.Resampling.BICUBIC)
    return out


def build_frame(
    *,
    outline: Sequence[Tuple[float, float]],
    pepper_mask: Image.Image,
    inner_mask: Image.Image,
    base_pepper_black: Image.Image,
    base_pepper_transparent: Image.Image,
    text_layer: Image.Image,
    cover_slice: Image.Image,
    hinge_y: int,
    frame_idx: int,
) -> Image.Image:
    # background is solid black for animation
    frame = base_pepper_black.copy()

    # Determine phase
    if frame_idx < FRAMES_SOLID:
        # Step 1: solid form only
        return frame

    idx = frame_idx - FRAMES_SOLID
    if idx < FRAMES_KNIFE:
        # Step 2: knife slices
        t = (idx + 1) / FRAMES_KNIFE
        draw_knife_line(frame, outline, t)
        return frame

    idx -= FRAMES_KNIFE
    if idx < FRAMES_FALL:
        # Step 3-4: cover slice tips forward; reveals text top-down
        t = (idx + 1) / FRAMES_FALL

        # Reveal mask: show text from top to bottom
        reveal = clamp(piecewise_tilt_weighted(t), 0.0, 1.0)
        bbox = inner_mask.getbbox() or (0, 0, FRAME_W, FRAME_H)
        ix0, iy0, ix1, iy1 = bbox
        ry = int(lerp(iy0, iy1, reveal))
        reveal_mask = Image.new("L", (FRAME_W, FRAME_H), 0)
        ImageDraw.Draw(reveal_mask).rectangle([0, 0, FRAME_W, ry], fill=255)
        clipped_reveal = ImageChops.multiply(inner_mask, reveal_mask)

        revealed_text = text_layer.copy()
        a = revealed_text.split()[-1]
        revealed_text.putalpha(ImageChops.multiply(a, clipped_reveal))

        frame.alpha_composite(revealed_text)

        # Transform cover slice (domino fall)
        slice_fallen = transform_domino_fall(cover_slice, hinge_y, t)
        frame.alpha_composite(slice_fallen)
        return frame

    idx -= FRAMES_FALL
    # Step 5: dissolve cover away, leave completed logo
    t = (idx + 1) / FRAMES_DISSOLVE
    frame = base_pepper_black.copy()
    frame.alpha_composite(text_layer)

    # Optional small remnants dissolving (fast)
    if t < 1.0:
        slice_alpha = int(255 * (1.0 - ease_out_quad(t)))
        slice_fallen = transform_domino_fall(cover_slice, hinge_y, 1.0)
        if slice_alpha > 0:
            sa = slice_fallen.split()[-1]
            sa = ImageChops.multiply(sa, Image.new("L", sa.size, slice_alpha))
            slice_fallen.putalpha(sa)
            frame.alpha_composite(slice_fallen)
    return frame


def make_spritesheet(frames: Sequence[Image.Image], cols: int, bg: Color) -> Tuple[Image.Image, int, int]:
    if not frames:
        raise ValueError("no frames")
    fw, fh = frames[0].size
    cols = max(1, cols)
    rows = math.ceil(len(frames) / cols)
    sheet = Image.new("RGBA", (fw * cols, fh * rows), bg.rgba())
    for idx, fr in enumerate(frames):
        x = (idx % cols) * fw
        y = (idx // cols) * fh
        sheet.paste(fr, (x, y))
    return sheet, cols, rows


def svg_path_from_points(points: Sequence[Tuple[float, float]]) -> str:
    if not points:
        return ""
    parts = [f"M {points[0][0]:.2f} {points[0][1]:.2f}"]
    for x, y in points[1:]:
        parts.append(f"L {x:.2f} {y:.2f}")
    parts.append("Z")
    return " ".join(parts)


def write_logo_svg(path: Path, outline: Sequence[Tuple[float, float]]) -> None:
    """
    Simple flat SVG that matches the rendered PNG. Text is clipped to a slightly
    shrunken inner cavity. (Envelope-like warping is handled in the PNG render.)
    """
    pepper_path = svg_path_from_points(outline)

    # Inner cavity by applying a stroke-only inset approximation (via clipPath + scale)
    # We'll approximate inner cavity by reusing the same path with a uniform scale
    # around the pepper's center.
    minx, miny, maxx, maxy = points_bounds(outline)
    cx = (minx + maxx) * 0.5
    cy = (miny + maxy) * 0.5
    sx = 0.86
    sy = 0.82

    svg = f"""<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="{FRAME_W}" height="{FRAME_H}" viewBox="0 0 {FRAME_W} {FRAME_H}">
  <defs>
    <clipPath id="pepperClip">
      <path d="{pepper_path}" />
    </clipPath>
    <clipPath id="innerClip">
      <g transform="translate({cx:.2f} {cy:.2f}) scale({sx:.4f} {sy:.4f}) translate({-cx:.2f} {-cy:.2f})">
        <path d="{pepper_path}" />
      </g>
    </clipPath>
  </defs>

  <!-- transparent background -->
  <!-- stem -->
  <g>
    <path d="M {minx - (maxx - minx) * 0.12:.2f} {cy - (maxy - miny) * 0.10:.2f}
             L {minx + (maxx - minx) * 0.02:.2f} {cy + (maxy - miny) * 0.02:.2f}"
          stroke="rgb({STEM_DARK.r},{STEM_DARK.g},{STEM_DARK.b})"
          stroke-width="{(maxy - miny) * 0.18:.2f}"
          stroke-linecap="round" fill="none"/>
  </g>

  <!-- pepper body -->
  <path d="{pepper_path}" fill="rgb({PEPPER_GREEN.r},{PEPPER_GREEN.g},{PEPPER_GREEN.b})" />

  <!-- inner text (clipped) -->
  <g clip-path="url(#innerClip)">
    <text x="{cx:.2f}" y="{cy:.2f}"
          text-anchor="middle" dominant-baseline="middle"
          font-family="Noto Sans, Arial, Helvetica, sans-serif"
          font-weight="800"
          font-size="140"
          fill="#FFFFFF"
          letter-spacing="2">{TEXT}</text>
  </g>
</svg>
"""
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(svg, encoding="utf-8")


def main() -> None:
    outline = pepper_outline_points()

    pepper_mask = draw_pepper_mask((FRAME_W, FRAME_H), outline)
    inner_mask = make_inner_mask(pepper_mask, shrink_px=int(min(FRAME_W, FRAME_H) * 0.06))

    # Base pepper renders
    base_pepper_black = draw_pepper_solid((FRAME_W, FRAME_H), outline, BG_BLACK)
    base_pepper_transparent = draw_pepper_solid((FRAME_W, FRAME_H), outline, None)

    text_layer = make_text_layer((FRAME_W, FRAME_H), inner_mask, outline)

    cover_slice, hinge_y = make_cover_slice((FRAME_W, FRAME_H), pepper_mask, outline)

    # Build animation frames
    frames: List[Image.Image] = []
    for i in range(TOTAL_FRAMES):
        fr = build_frame(
            outline=outline,
            pepper_mask=pepper_mask,
            inner_mask=inner_mask,
            base_pepper_black=base_pepper_black,
            base_pepper_transparent=base_pepper_transparent,
            text_layer=text_layer,
            cover_slice=cover_slice,
            hinge_y=hinge_y,
            frame_idx=i,
        )
        frames.append(fr)

    # Output spritesheet + metadata
    sheet, cols, rows = make_spritesheet(frames, SHEET_COLS, BG_BLACK)
    out_sheet = Path("resources/images/choicehoney_logo_anim_spritesheet.png")
    out_sheet.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(out_sheet)

    out_meta = out_sheet.with_suffix(".json")
    meta = {
        "name": "choicehoney_logo_anim",
        "frameWidth": FRAME_W,
        "frameHeight": FRAME_H,
        "frameCount": len(frames),
        "fps": FPS,
        "columns": cols,
        "rows": rows,
        "background": "#000000",
        "notes": {
            "sequence": [
                "solid pepper silhouette (no text)",
                "knife outline slices horizontally",
                "cover slice tips forward like a domino (2.5D)",
                "text reveals top-to-bottom as wipe",
                "slice dissolves away leaving final logo",
            ]
        },
    }
    out_meta.write_text(json.dumps(meta, indent=2), encoding="utf-8")

    # Output final logo (transparent background)
    final_logo_png = Path("resources/ui/choicehoney_logo_final.png")
    final_logo_png.parent.mkdir(parents=True, exist_ok=True)
    final_logo = base_pepper_transparent.copy()
    final_logo.alpha_composite(text_layer)
    final_logo.save(final_logo_png)

    final_logo_svg = Path("resources/ui/choicehoney_logo_final.svg")
    write_logo_svg(final_logo_svg, outline)

    print(f"wrote {out_sheet} ({sheet.size}), {out_meta}")
    print(f"wrote {final_logo_png} ({final_logo.size}), {final_logo_svg}")


if __name__ == "__main__":
    main()

