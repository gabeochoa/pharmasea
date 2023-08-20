
#pragma once

#include <string>

#include "../vendor_include.h"

inline int* CodepointRemoveDuplicates(int* codepoints, int codepointCount,
                                      int* codepointsResultCount) {
    int codepointsNoDupsCount = codepointCount;
    int* codepointsNoDups = (int*) calloc(codepointCount, sizeof(int));
    memcpy(codepointsNoDups, codepoints, codepointCount * sizeof(int));

    // Remove duplicates
    for (int i = 0; i < codepointsNoDupsCount; i++) {
        for (int j = i + 1; j < codepointsNoDupsCount; j++) {
            if (codepointsNoDups[i] == codepointsNoDups[j]) {
                for (int k = j; k < codepointsNoDupsCount; k++)
                    codepointsNoDups[k] = codepointsNoDups[k + 1];

                codepointsNoDupsCount--;
                j--;
            }
        }
    }

    // NOTE: The size of codepointsNoDups is the same as original array but
    // only required positions are filled (codepointsNoDupsCount)

    *codepointsResultCount = codepointsNoDupsCount;
    return codepointsNoDups;
}

inline raylib::Font load_font_for_string(const std::string& content,
                                         const std::string& font_filename,
                                         int size = 96) {
    int codepointCount = 0;
    int* codepoints = raylib::LoadCodepoints(content.c_str(), &codepointCount);

    int codepointNoDupsCounts = 0;
    int* codepointsNoDups = CodepointRemoveDuplicates(
        codepoints, codepointCount, &codepointNoDupsCounts);

    raylib::UnloadCodepoints(codepoints);

    return raylib::LoadFontEx(font_filename.c_str(), size, codepointsNoDups,
                              codepointNoDupsCounts);
}
