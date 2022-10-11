
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

const std::string ASSET_PACK = "packed_assets.bin";
const std::string PROGRAM_BINARY = "pharmasea";

namespace fs = std::filesystem;

static unsigned int elf_hash(std::string str) {
    unsigned int hash = 0;
    unsigned int x = 0;
    unsigned int i = 0;
    unsigned int len = str.length();
    for (i = 0; i < len; i++) {
        hash = (hash << 4) + (str[i]);
        if ((x = hash & 0xF0000000) != 0) {
            hash ^= (x >> 24);
        }
        hash &= ~x;
    }

    return hash;
}

struct FileInfo {
    std::vector<std::string> files;
    FileInfo() {}
    int num_files() const { return files.size(); }
    std::string get(int i) const { return files[i]; }
    void add(std::string file) { files.push_back(file); }

} fileinfo;

void gather_resources_for_path(fs::path path) {
    for (auto const& dir_entry : fs::recursive_directory_iterator{path}) {
        if (!dir_entry.is_regular_file()) {
            continue;
        }
        std::cout << "Adding resource: " << dir_entry.path() << '\n';
        fileinfo.add(dir_entry.path().string());
    }
}

void package_into_binary() {
    const unsigned int buffer_size = 2048;
    unsigned char buffer[buffer_size];
    FILE* output = fopen(ASSET_PACK.c_str(), "w");
    int index_element_size = sizeof(unsigned int) * 3;
    int index_size = index_element_size * fileinfo.num_files();
    fwrite(&index_size, sizeof(index_size), 1, output);

    unsigned int index_offset = sizeof(unsigned int);
    unsigned int current_blob_offset = 0;

    /* Write all the files to the output file. */
    for (int i = 0; i < fileinfo.num_files(); i++) {
        unsigned int file_size;
        unsigned int name_hash;

        name_hash = elf_hash(fileinfo.get(i));

        FILE* file = fopen(fileinfo.get(i).c_str(), "r");

        fseek(file, 0, SEEK_END);
        file_size = ftell(file);

        /* Write the file into the index */
        fseek(output, index_element_size * i + index_offset, SEEK_SET);
        fwrite(&name_hash, sizeof(name_hash), 1, output);
        fwrite(&current_blob_offset, sizeof(current_blob_offset), 1, output);
        fwrite(&file_size, sizeof(file_size), 1, output);

        /* Copy the file's data into the data blob */
        fseek(output, current_blob_offset + index_size + index_offset,
              SEEK_SET);
        fseek(file, 0, SEEK_SET);
        for (unsigned int ii = 0; ii < file_size; ii += buffer_size) {
            unsigned int bytes_read = fread(buffer, 1, buffer_size, file);
            fwrite(buffer, bytes_read, 1, output);
        }
        fclose(file);
        current_blob_offset += file_size;
    }

    fclose(output);
}

void append_onto_binary() {
    const unsigned int buffer_size = 2048;
    unsigned char buffer[buffer_size];

    FILE* exec_file = fopen(PROGRAM_BINARY.c_str(), "r+");
    FILE* package_file = fopen(ASSET_PACK.c_str(), "r");

    fseek(exec_file, 0, SEEK_END);
    fseek(package_file, 0, SEEK_END);
    unsigned int package_size = ftell(package_file);

    for (unsigned int i = 0; i < package_size; i += buffer_size) {
        unsigned int bytes_read = fread(buffer, 1, buffer_size, package_file);
        fwrite(buffer, bytes_read, 1, exec_file);
    }

    fclose(exec_file);
    fclose(package_file);
}

void gather_resources() {  //
    gather_resources_for_path(fs::path("../resources/"));
}

int main() {
    gather_resources();
    package_into_binary();
    append_onto_binary();
    return 0;
}
