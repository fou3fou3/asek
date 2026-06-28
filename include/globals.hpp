#pragma once
#include <format>

constexpr const char *DATA_DIR = "data";
constexpr const char *INDEX_DIR = "index";
const std::string TFIDF_INDEX_PATH =
    std::format("{}/tfIdfIndex.bin", INDEX_DIR);