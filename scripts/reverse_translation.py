
import re

def reverse_words_outside_braces(match):
    m = match.group(1)
    if "engine/" in m:
        return m

    if "strings.h" in m:
        return m

    if m[0] != "\"":
        return m
    sentence = m[1:-1]

    # Find the substring inside the curly braces
    match = re.search(r'{[^}]*}', sentence)
    if match:
        # Extract the substring inside the curly braces
        inside_braces = match.group(0)

        # Split the sentence into three parts: before, inside, and after the curly braces
        before_braces, _, after_braces = sentence.partition(inside_braces)

        # Reverse the letters in the words in the before and after parts
        before_braces = ' '.join(word[::-1] for word in before_braces.split())
        after_braces = ' '.join(word[::-1] for word in after_braces.split())

        # Combine the parts back into a sentence
        return f"\"{before_braces}{inside_braces}{after_braces}\""
    else:
        # If there are no curly braces, just reverse the letters in the words
        return "\"" + ' '.join(word[::-1] for word in sentence.split()) + "\""

# Function to reverse strings inside quotes while ignoring curly braces content
def reverse_quotes(match):
    string = match.group(1)
    print(string)
    if string[0] != '"':
        return string;
    output = '"' + ''.join(reversed(string[1:-1])) + '"'

    return output;


if __name__ == "__main__":
    input_file = "src/translation_en_us.h"
    output_file = "src/translations_en_rev.h"


    # Define a regex pattern to match strings inside quotes
    pattern = r'("[^{}"]*(?:{[^{}]*}[^{}"]*)*")'

    with open(input_file, 'r') as f:
        input_string = f.read()

    input_string = input_string.replace("en_us", "en_rev")

    # Replace the matched strings with the reversed ones
    output_string = re.sub(pattern, reverse_words_outside_braces, input_string)

    print(output_string);


