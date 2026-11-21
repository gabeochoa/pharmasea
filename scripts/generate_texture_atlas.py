#!/usr/bin/env python3
"""
Generate texture atlases from folders of images.

Usage:
    python generate_texture_atlas.py <input_folder> <output_name> [--size SIZE]

Example:
    python generate_texture_atlas.py resources/images/controls/keyboard_default keyboard_atlas --size 2048
    
Outputs:
    - resources/images/<output_name>.png (atlas texture)
    - resources/config/<output_name>.json (region manifest)

Requires: Pillow (pip install Pillow)
"""

import argparse
import json
import math
from pathlib import Path
from PIL import Image


def get_images_from_folder(folder: Path) -> list[tuple[str, Image.Image]]:
    """Load all PNG images from a folder, returning (name, image) tuples."""
    images = []
    for file in sorted(folder.glob("*.png")):
        name = file.stem  # filename without extension
        img = Image.open(file).convert("RGBA")
        images.append((name, img))
    return images


def calculate_atlas_layout(images: list[tuple[str, Image.Image]], max_size: int) -> tuple[int, int, int, int]:
    """
    Calculate optimal atlas dimensions.
    Returns (atlas_width, atlas_height, cell_width, cell_height)
    """
    if not images:
        raise ValueError("No images to pack")
    
    # Get max dimensions from all images
    max_w = max(img.width for _, img in images)
    max_h = max(img.height for _, img in images)
    
    # Calculate how many cells fit
    cells_per_row = max_size // max_w
    num_rows = math.ceil(len(images) / cells_per_row)
    
    # Final atlas dimensions
    atlas_w = cells_per_row * max_w
    atlas_h = num_rows * max_h
    
    # Ensure we don't exceed max_size
    if atlas_h > max_size:
        # Need to recalculate with more columns or smaller cells
        # For now, just warn and proceed
        print(f"Warning: Atlas height {atlas_h} exceeds max size {max_size}")
    
    return atlas_w, atlas_h, max_w, max_h


def pack_atlas(images: list[tuple[str, Image.Image]], max_size: int) -> tuple[Image.Image, dict]:
    """
    Pack images into an atlas using simple grid layout.
    Returns (atlas_image, regions_dict)
    """
    if not images:
        raise ValueError("No images to pack")
    
    atlas_w, atlas_h, cell_w, cell_h = calculate_atlas_layout(images, max_size)
    cells_per_row = atlas_w // cell_w
    
    # Create atlas
    atlas = Image.new("RGBA", (atlas_w, atlas_h), (0, 0, 0, 0))
    regions = {}
    
    for idx, (name, img) in enumerate(images):
        col = idx % cells_per_row
        row = idx // cells_per_row
        
        x = col * cell_w
        y = row * cell_h
        
        # Center the image in its cell if smaller than cell size
        offset_x = (cell_w - img.width) // 2
        offset_y = (cell_h - img.height) // 2
        
        atlas.paste(img, (x + offset_x, y + offset_y), img)
        
        # Store region with actual image bounds (not cell bounds)
        regions[name] = {
            "x": x + offset_x,
            "y": y + offset_y,
            "w": img.width,
            "h": img.height
        }
    
    return atlas, regions


def generate_atlas(input_folder: Path, output_name: str, max_size: int = 2048):
    """Generate an atlas from a folder of images."""
    
    print(f"Loading images from {input_folder}...")
    images = get_images_from_folder(input_folder)
    print(f"Found {len(images)} images")
    
    if not images:
        print("No images found, skipping")
        return
    
    print(f"Packing atlas (max size {max_size})...")
    atlas, regions = pack_atlas(images, max_size)
    
    # Output paths
    atlas_path = Path("resources/images") / f"{output_name}.png"
    manifest_path = Path("resources/config") / f"{output_name}.json"
    
    # Ensure directories exist
    atlas_path.parent.mkdir(parents=True, exist_ok=True)
    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    
    # Save atlas image
    atlas.save(atlas_path, optimize=True)
    print(f"Saved atlas: {atlas_path} ({atlas.width}x{atlas.height})")
    
    # Save manifest
    manifest = {
        "texture": f"{output_name}.png",
        "width": atlas.width,
        "height": atlas.height,
        "regions": regions
    }
    with open(manifest_path, "w") as f:
        json.dump(manifest, f, indent=2)
    print(f"Saved manifest: {manifest_path} ({len(regions)} regions)")


def main():
    parser = argparse.ArgumentParser(description="Generate texture atlas from folder of images")
    parser.add_argument("input_folder", type=Path, help="Folder containing PNG images")
    parser.add_argument("output_name", type=str, help="Output atlas name (without extension)")
    parser.add_argument("--size", type=int, default=2048, help="Maximum atlas size (default: 2048)")
    
    args = parser.parse_args()
    
    if not args.input_folder.exists():
        print(f"Error: Input folder does not exist: {args.input_folder}")
        return 1
    
    generate_atlas(args.input_folder, args.output_name, args.size)
    return 0


if __name__ == "__main__":
    exit(main())

