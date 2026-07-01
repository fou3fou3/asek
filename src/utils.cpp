#include "utils.hpp"
#include "flatbuffers/flatbuffers.h"
#include <fstream>
#include <iostream>
#include <vector>

void save_flatbuffer_to_disk(flatbuffers::FlatBufferBuilder &builder,
                             const std::string &filename) {
  uint8_t *buf = builder.GetBufferPointer();
  size_t size = builder.GetSize();

  std::ofstream outfile(filename, std::ios::binary | std::ios::out);

  if (!outfile.is_open()) {
    std::cerr << "Failed to open file for writing: " << filename << std::endl;
    return;
  }

  outfile.write(reinterpret_cast<const char *>(buf), size);
  outfile.close();
}

std::vector<char> load_flatbuffer_from_disk(const std::string &filename) {
  std::ifstream infile(filename, std::ios::binary | std::ios::ate);

  if (!infile.is_open()) {
    std::cerr << "Failed to open file for reading: " << filename << std::endl;
    return {};
  }

  std::streamsize size = infile.tellg();
  infile.seekg(0, std::ios::beg);

  std::vector<char> buffer(size);

  if (!infile.read(buffer.data(), size)) {
    std::cerr << "Error reading file data." << std::endl;
  }

  return buffer;
}