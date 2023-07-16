
import re

def extract_strings_from_cpp(input_file):
    # Regular expression to match C++ string literals within the i18n namespace
    string_pattern = r'namespace\s+i18n\s*{\s*(.*?)\s*}'

    with open(input_file, 'r') as f:
        content = f.read()

    # Find all matches of the namespace pattern
    matches = re.findall(string_pattern, content, re.DOTALL)

    if not matches:
        print("No strings found in the i18n namespace.")
        return []

    i18n_content = matches[0]

    # Regular expression to match C++ string literals
    string_pattern = r'\"(.*?)\"'
    extracted_strings = re.findall(string_pattern, i18n_content, re.DOTALL)

    #  for index, s in enumerate(extracted_strings, 1):
        #  print(f"String {index}: {s}")

    return extracted_strings

def write_to_po_file(strings, output_file):
    with open(output_file, 'w') as f:
        for index, s in enumerate(strings, 1):
            r = s[::-1]
            f.write(f'msgid "{s}"\n')
            f.write(f'msgstr "{r}"\n')
            f.write('\n')

if __name__ == "__main__":
    input_file = "src/strings.h"
    output_file = "resources/translations/en_rev.po"

    extracted_strings = extract_strings_from_cpp(input_file)
    write_to_po_file(extracted_strings, output_file)
