#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

void read_words_frequencies_from_buffer(
    const std::vector<char> &buffer,
    std::unordered_map<std::string, uint32_t> &wordsFrequencies,
    uint32_t &numberOfWordsInDocument);

void get_words_frequencies_from_document(
    const std::filesystem::directory_entry &entry,
    std::unordered_map<std::string, uint32_t> &wordsFrequencies,
    uint32_t &numberOfWordsInDocument);

size_t calculate_tf_of_all_words(
    const std::filesystem::path &dirPath,
    std::unordered_map<std::string, std::unordered_map<std::string, double>>
        &tfIdfOfAllWords,
    std::unordered_map<std::string, double> &documentsMagnitudes);

void index_tf_idf();
void index();