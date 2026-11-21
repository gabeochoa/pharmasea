#!/usr/bin/env python3
"""
Convert GLB/GLTF files to OBJ format for raylib 5.5 compatibility.

This script finds all GLB and GLTF files in the models directory and converts
them to OBJ format, preserving the folder structure in models/converted/.
"""

import os
import sys
import argparse
from pathlib import Path

def check_and_install_trimesh():
    """Check if trimesh is available, install if not."""
    try:
        import trimesh
        return trimesh
    except ImportError:
        print("trimesh not found.")
        print("\nThis script requires the 'trimesh' library to convert GLB/GLTF files.")
        print("Please install it using one of these methods:\n")
        print("  Option 1 (recommended for system Python):")
        print("    python3 -m pip install --user trimesh")
        print("\n  Option 2 (if you have a virtual environment):")
        print("    source venv/bin/activate  # or your venv path")
        print("    pip install trimesh")
        print("\n  Option 3 (if you want to override system protection):")
        print("    python3 -m pip install --break-system-packages trimesh")
        print("\nAfter installing, run this script again.")
        return None

def convert_with_trimesh(input_path, output_path):
    """Convert GLB/GLTF to OBJ using trimesh."""
    try:
        import trimesh
        print(f"  Loading {input_path.name}...")
        scene = trimesh.load(str(input_path))
        
        # Handle both single meshes and scenes
        if isinstance(scene, trimesh.Scene):
            # For scenes, combine all meshes or export the first one
            if len(scene.geometry) > 0:
                # Get the first mesh (or combine them)
                mesh = list(scene.geometry.values())[0]
                if len(scene.geometry) > 1:
                    print(f"    Warning: Scene has {len(scene.geometry)} meshes, exporting first one")
            else:
                print(f"    Error: Scene has no geometry")
                return False
        elif isinstance(scene, trimesh.Trimesh):
            mesh = scene
        else:
            print(f"    Error: Unexpected type: {type(scene)}")
            return False
        
        # Ensure output directory exists
        output_path.parent.mkdir(parents=True, exist_ok=True)
        
        # Export as OBJ
        print(f"  Exporting to {output_path}...")
        mesh.export(str(output_path))
        print(f"  ✓ Successfully converted {input_path.name}")
        return True
        
    except Exception as e:
        print(f"  ✗ Error converting {input_path.name}: {e}")
        return False

def find_gltf_files(models_dir):
    """Find all GLB and GLTF files in the models directory."""
    models_path = Path(models_dir)
    gltf_files = []
    
    for ext in ['*.glb', '*.gltf']:
        gltf_files.extend(models_path.rglob(ext))
    
    return sorted(gltf_files)

def get_output_path(input_path, models_dir, output_base):
    """Calculate output path preserving folder structure."""
    models_path = Path(models_dir)
    output_base_path = Path(output_base)
    
    # Get relative path from models directory
    try:
        rel_path = input_path.relative_to(models_path)
    except ValueError:
        # If file is not under models_dir, just use filename
        rel_path = Path(input_path.name)
    
    # Change extension to .obj
    obj_name = rel_path.with_suffix('.obj')
    
    # Create output path
    output_path = output_base_path / obj_name
    
    return output_path

def main():
    parser = argparse.ArgumentParser(
        description='Convert GLB/GLTF files to OBJ format'
    )
    parser.add_argument(
        '--models-dir',
        default='resources/models',
        help='Directory containing GLB/GLTF files (default: resources/models)'
    )
    parser.add_argument(
        '--output-dir',
        default='resources/models/converted',
        help='Output directory for OBJ files (default: resources/models/converted)'
    )
    parser.add_argument(
        '--dry-run',
        action='store_true',
        help='Show what would be converted without actually converting'
    )
    
    args = parser.parse_args()
    
    # Resolve paths relative to script location
    script_dir = Path(__file__).parent.parent
    models_dir = script_dir / args.models_dir
    output_dir = script_dir / args.output_dir
    
    if not models_dir.exists():
        print(f"Error: Models directory not found: {models_dir}")
        sys.exit(1)
    
    # Find all GLB/GLTF files
    print(f"Searching for GLB/GLTF files in {models_dir}...")
    gltf_files = find_gltf_files(models_dir)
    
    if not gltf_files:
        print("No GLB/GLTF files found.")
        return
    
    print(f"Found {len(gltf_files)} GLB/GLTF file(s) to convert\n")
    
    if args.dry_run:
        print("DRY RUN - Files that would be converted:")
        for gltf_file in gltf_files:
            output_path = get_output_path(gltf_file, models_dir, output_dir)
            print(f"  {gltf_file} -> {output_path}")
        return
    
    # Check for trimesh
    trimesh = check_and_install_trimesh()
    if trimesh is None:
        print("\nError: Could not import trimesh. Please install it manually:")
        print("  pip install trimesh")
        sys.exit(1)
    
    # Convert files
    success_count = 0
    fail_count = 0
    
    for gltf_file in gltf_files:
        output_path = get_output_path(gltf_file, models_dir, output_dir)
        print(f"\n[{success_count + fail_count + 1}/{len(gltf_files)}] Converting {gltf_file.name}...")
        
        if convert_with_trimesh(gltf_file, output_path):
            success_count += 1
        else:
            fail_count += 1
    
    # Summary
    print(f"\n{'='*60}")
    print(f"Conversion complete!")
    print(f"  ✓ Successfully converted: {success_count}")
    print(f"  ✗ Failed: {fail_count}")
    print(f"  Output directory: {output_dir}")
    print(f"{'='*60}")

if __name__ == '__main__':
    main()


ka 