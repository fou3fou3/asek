#include "globals.hpp"
#include "tfIdfIndex_generated.h"
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

void SaveFlatBufferToDisk(flatbuffers::FlatBufferBuilder &builder,
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

void read_words_frequencies_from_buffer(
    const std::vector<char> &buffer,
    std::unordered_map<std::string, uint32_t> &wordsFrequencies,
    uint32_t &numberOfWordsInDocument) {
  const char *ptr = buffer.data();
  const char *end = ptr + buffer.size();

  size_t wordCount = 0;

  while (ptr < end) {
    while (ptr < end && std::isspace(static_cast<unsigned char>(*ptr))) {
      ptr++;
    }

    if (ptr == end)
      break;

    const char *wordStart = ptr;
    while (ptr < end && !std::isspace(static_cast<unsigned char>(*ptr))) {
      ptr++;
    }
    const char *wordEnd = ptr;

    std::string word(wordStart, wordEnd);

    std::string realWord;

    for (char c : word) {
      if (std::isalpha(static_cast<unsigned char>(c)))
        realWord += std::tolower(c);
    }

    if (!(realWord == "")) {
      wordsFrequencies[realWord] += 1;
      wordCount++;
    }
  }

  numberOfWordsInDocument = wordCount;
}

void get_words_frequencies_from_document(
    const std::filesystem::directory_entry &entry,
    std::unordered_map<std::string, uint32_t> &wordsFrequencies,
    uint32_t &numberOfWordsInDocument) {

  std::ifstream document(entry.path(), std::ios::binary | std::ios::ate);

  if (!document.is_open()) {
    std::cerr << "Error: Could not open the file!" << std::endl;
  }

  std::streamsize size = document.tellg();
  document.seekg(0, std::ios::beg);

  std::vector<char> buffer(size);
  if (document.read(buffer.data(), size)) {
    read_words_frequencies_from_buffer(buffer, wordsFrequencies,
                                       numberOfWordsInDocument);
  }

  document.close();
}

size_t calculate_tf_of_all_words(
    const std::filesystem::path &dirPath,
    std::unordered_map<std::string, std::unordered_map<std::string, double>>
        &tfIdfOfAllWords) {
  size_t numberOfDocuments = 0;

  if (std::filesystem::exists(dirPath) &&
      std::filesystem::is_directory(dirPath)) {
    for (const auto &entry : std::filesystem::directory_iterator(dirPath)) {
      try {
        if (std::filesystem::exists(entry.path()) &&
            std::filesystem::is_regular_file(entry)) {

          std::unordered_map<std::string, uint32_t> documentWordsFrequencies;
          uint32_t numberOfWordsInDocument;
          numberOfDocuments++;

          std::string fileName = entry.path().filename().string();

          get_words_frequencies_from_document(entry, documentWordsFrequencies,
                                              numberOfWordsInDocument);

          for (auto wordFrequency : documentWordsFrequencies) {
            tfIdfOfAllWords[wordFrequency.first][fileName] =
                (1.0f * wordFrequency.second) / numberOfWordsInDocument;
            ;
          }
        }
      } catch (const std::filesystem::filesystem_error &e) {
        std::cerr << "Error: " << e.what() << "\n";
      }

      std::cout << entry.path() << "\n";
    }
  }

  return numberOfDocuments;
}

void index_tf_idf() {
  std::filesystem::path dirPath = DATA_DIR;
  std::unordered_map<std::string, std::unordered_map<std::string, double>>
      tfOfAllWords;
  size_t numberOfDocuments;
  std::unordered_map<std::string, double> documentsTfIdfSumSquared;

  try {
    numberOfDocuments = calculate_tf_of_all_words(dirPath, tfOfAllWords);
  } catch (const std::filesystem::filesystem_error &e) {
    std::cerr << "Error: " << e.what() << "\n";
  }

  flatbuffers::FlatBufferBuilder builder(4096);
  std::vector<flatbuffers::Offset<tfIdfIndex::WordsToDocumentsTfIdf>>
      tfIdfIndexContainer;

  for (const auto &[word, documentsTf] : tfOfAllWords) {

    std::vector<flatbuffers::Offset<tfIdfIndex::DocumentTfIdf>>
        documentsTfIdfContainer;
    double idf = std::log10(1.0f * numberOfDocuments / documentsTf.size());

    for (auto &document : documentsTf) {
      double tfIdfResult = document.second * idf;

      auto inner_entry = tfIdfIndex::CreateDocumentTfIdf(
          builder, builder.CreateString(document.first), tfIdfResult);
      documentsTfIdfContainer.push_back(inner_entry);

      documentsTfIdfSumSquared[document.first] += std::pow(tfIdfResult, 2);
    }

    auto sortedDocumentsTfIdfContainer =
        builder.CreateVectorOfSortedTables(&documentsTfIdfContainer);

    auto wordTfIdfs = tfIdfIndex::CreateWordsToDocumentsTfIdf(
        builder, builder.CreateString(word), sortedDocumentsTfIdfContainer);
    tfIdfIndexContainer.push_back(wordTfIdfs);
  }
  auto sortedTfIdfIndexContainer =
      builder.CreateVectorOfSortedTables(&tfIdfIndexContainer);

  std::vector<flatbuffers::Offset<tfIdfIndex::DocumentMagnitude>>
      documentsMagnitudesContainer;
  for (const auto &[str_val, dbl_val] : documentsTfIdfSumSquared) {

    auto documentTfIdfSum = tfIdfIndex::CreateDocumentMagnitude(
        builder, builder.CreateString(str_val), std::sqrt(dbl_val));

    documentsMagnitudesContainer.push_back(documentTfIdfSum);
  }
  auto finalDocumentsMagnitudesContainer =
      builder.CreateVectorOfSortedTables(&documentsMagnitudesContainer);

  auto root_payload = tfIdfIndex::CreateMainPayload(
      builder, sortedTfIdfIndexContainer, numberOfDocuments,
      finalDocumentsMagnitudesContainer);
  builder.Finish(root_payload);

  uint8_t *buf = builder.GetBufferPointer();

  SaveFlatBufferToDisk(builder, TFIDF_INDEX_PATH);
}

void index() {
  std::cout << "INDEXING!\n";
  index_tf_idf();
}
