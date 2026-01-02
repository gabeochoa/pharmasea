import re
import os
import sys
from pathlib import Path

from collections import defaultdict



base_classes = [
    "BaseComponent",
    "AIComponent",
    "CanBeHeld",
]

mapping = defaultdict(list)

# Define the directory to search for component files
component_dir = "src/components"
legacy_network_file = "src/network/polymorphic_components.h"


def _resolve_network_file():
    """
    Historically we tracked Bitsery polymorphic registration in
    `src/network/polymorphic_components.h` and this script ensured every
    `struct X : public BaseComponent` appeared in that list.

    The project has since moved to pointer-free snapshot serialization, and the
    legacy header may no longer exist. In that case this check should be a no-op
    so builds don't fail.
    """
    p = Path(legacy_network_file)
    return str(p) if p.exists() else None


def collect_existing():
    network_file = _resolve_network_file()
    if not network_file:
        # Pointer-free snapshot serialization no longer needs the legacy
        # polymorphic registration list.
        return

    # Open the file and read its contents
    with open(network_file, "r") as file:
        contents = file.read()

    pattern = r'PolymorphicBaseClass<(\w+)>\s*:.*?// BEGIN\n(.*?)// END'
    matches = re.findall(pattern, contents, re.DOTALL)

    for match in matches:
        base_class = match[0]
        derived_classes = [cls.strip() for cls in match[1].split(',')]
        for cls in derived_classes:
            mapping[base_class].append(cls)

def find_missing():
    missing = 0;
    for filename in os.listdir(component_dir):
        # Open the file and read its contents
        with open(os.path.join(component_dir, filename), "r") as file:
            contents = file.read().replace("\n", "")

            # Loop through each base class
            for base_class in base_classes:
                base_map = mapping[base_class]

                # Use regular expressions to find any classes that inherit from the base class
                pattern = r"struct\s+(\w+)\s*:\s*public\s+" + base_class
                matches = re.findall(pattern, contents)

                # If any matches are found, print the name of the class
                for match in matches:
                    if match not in base_map:
                        print(f"Found class {match} missing from {base_class}")
                        missing +=1
    return missing


if __name__ == "__main__":
    # find existing components
    collect_existing();
    # If we didn't load a mapping (no legacy file), skip the check.
    if all(len(mapping[b]) == 0 for b in base_classes):
        sys.exit(0)
    # Loop through each file in the component directory
    res = find_missing();
    if(res > 0):
        sys.exit(res)
