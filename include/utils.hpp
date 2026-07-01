#pragma once
#include "flatbuffers/flatbuffers.h"
#include <string>
#include <vector>

void save_flatbuffer_to_disk(flatbuffers::FlatBufferBuilder &builder,
                             const std::string &filename);
std::vector<char> load_flatbuffer_from_disk(const std::string &filename);