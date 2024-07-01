import re
import os
import sys

from collections import defaultdict



base_classes = [
    "BaseComponent",
    "AIComponent"
]

mapping = defaultdict(list)

# Define the directory to search for component files
component_dir = "src/components"
network_file = "src/network/polymorphic_components.h"


def collect_existing():
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
    # Loop through each file in the component directory
    res = find_missing();
    if(res > 0):
        sys.exit(res)
