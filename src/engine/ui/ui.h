
#pragma once

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "../util.h"

std::string loadFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

struct UIElement {
    std::string tag;  // HTML tag, e.g., "div", "button", "h1", etc.
    std::string
        text;  // Text content for text-based elements like buttons or headings
    std::vector<UIElement> children;  // Child elements (nested elements)
                                      //
    UIElement(const std::string& tag, const std::string& text)
        : tag(tag), text(text) {}
};

UIElement parseHTML(const std::string& uiMarkup) {
    std::vector<UIElement> stack;
    stack.push_back(UIElement("div", ""));  // Add a root element to the stack
    std::istringstream iss(uiMarkup);
    std::string line;

    while (std::getline(iss, line)) {
        // Split the line into tokens using space as the delimiter
        std::vector<std::string> tokens = util::split_string(line, " ");
        if (tokens.empty()) continue;  // Empty line

        // Extract the tag name and text content from the first token
        std::string tag = tokens[0];
        std::string text;
        if (tokens.size() > 1) {
            text = tokens[1];
        }

        // Create a new UI element
        UIElement element(tag, text);

        // Add the new element to its parent's children list
        stack.back().children.push_back(element);

        // Check if the current tag is a container element (e.g., div, span)
        // If so, push the element onto the stack to act as the new parent
        if (tag == "div" || tag == "span" || tag == "p" || tag == "h1" ||
            tag == "h2" || tag == "h3") {
            stack.push_back(element);
        }

        // If the tag is a closing tag, pop the corresponding container from the
        // stack
        if (tag[0] == '/' && stack.size() > 1 &&
            stack.back().tag == tag.substr(1)) {
            stack.pop_back();
        }
    }

    return stack[0];  // Return the root element
}

// Represents the style properties for a UI element
class Style {
   public:
    std::unordered_map<std::string, std::string> properties;

    // Function to apply a style to a UI element
    void applyTo(UIElement& element);
};

Style parseCSS(const std::string&) {
    // TODO: Implement the CSS parser to parse the styles and properties
    // For simplicity, we won't implement the full CSS parsing here.
    // Instead, we'll return an empty Style for the example.
    return Style();
}

void Style::applyTo(UIElement&) {
    // TODO: Apply the styles from this instance to the UI element
    // For simplicity, we won't implement applying styles here.
    // Instead, we'll return an empty Style for the example.
}

void performLayout(UIElement& root, float containerWidth,
                   float containerHeight) {
    // TODO: Implement layout calculations for the UI element tree
    // For simplicity, we won't implement the full layout engine here.
    // Instead, we'll just adjust the root element's position and size for the
    // example.
    // root.x = 10;
    // root.y = 10;
    // root.width = containerWidth - 20;
    // root.height = containerHeight - 20;
}

void renderElement(const UIElement& element, int indent) {
    std::string indentation(indent, ' ');
    std::cout << indentation << "<" << element.tag << ">" << std::endl;
    if (!element.text.empty()) {
        std::cout << indentation << "  " << element.text << std::endl;
    }

    for (const UIElement& child : element.children) {
        renderElement(child, indent + 2);
    }

    std::cout << indentation << "</" << element.tag << ">" << std::endl;
}

void renderUI(UIElement& root) {
    // TODO: Implement the rendering function to draw the UI element tree
    // For simplicity, we'll print the structure of the UI elements for the
    // example.
    std::cout << "Rendering UI:" << std::endl;
    renderElement(root, 0);
}

int ui_main() {
    // Load the UI markup and CSS styles from files or strings
    std::string uiMarkup = loadFile("ui_markup.html");
    std::string cssStyles = loadFile("styles.css");

    // Parse the UI markup and CSS styles
    UIElement rootElement = parseHTML(uiMarkup);
    Style style = parseCSS(cssStyles);

    // Apply styles to the UI element tree
    style.applyTo(rootElement);

    // Perform layout calculations
    int containerWidth = 0;
    int containerHeight = 0;

    performLayout(rootElement, containerWidth, containerHeight);

    // Render the UI
    renderUI(rootElement);

    return 0;
}
