#include "search.hpp"
#include "tfIdfIndex_generated.h"
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>


#include <unordered_map>

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

std::multiset<SearchResult, CompareSearchResults>
search_tfidf(const tfIdfIndex::MainPayload *tfIdfIndex, std::string query) {
  auto tfIdfWords = tfIdfIndex->tfidf_all_words();
  std::multiset<SearchResult, CompareSearchResults> searchResults;

  std::transform(query.begin(), query.end(), query.begin(),
                 [](unsigned char c) {
                   return std::tolower(c);
                 }); // Make all of the query lower-case

  std::unordered_map<std::string, float> queryWordsTfIdf;
  double querySquaredSum = 0;

  uint16_t numberOfWordsInQuery = 0;

  std::istringstream stream(query);
  std::string word;

  while (stream >> word) {
    queryWordsTfIdf[word] += 1;
    numberOfWordsInQuery += 1;
  }

  std::unordered_map<std::string, std::unordered_map<std::string, float>>
      documentsQueryWordsTfIdf;

  for (auto it = queryWordsTfIdf.begin(); it != queryWordsTfIdf.end();) {

    // Check if the word exists in our index
    if (auto word = tfIdfWords->LookupByKey(it->first)) {
      // Compute tf-idf of words in the query
      it->second = ((1.0f * it->second) / numberOfWordsInQuery) *
                   std::log10(1.0f * tfIdfIndex->number_of_documents() /
                              word->values()->size());

      querySquaredSum += std::pow(it->second, 2);

      // Fill documentsQueryWordsTfIdf
      for (const tfIdfIndex::DocumentTfIdf *documentTfIdf : *word->values()) {
        documentsQueryWordsTfIdf[documentTfIdf->key()->str()][it->first] =
            documentTfIdf->value();
      }

      ++it;
    } else {
      it = queryWordsTfIdf.erase(
          it); // Erase the word from the query because its not needed
    }
  }

  double queryMagnitude = std::sqrt(querySquaredSum);

  for (const auto &document : documentsQueryWordsTfIdf) {
    double queryDocumentMultipSum =
        0; // Sum of Qi * Di; where i is the word inde

    for (auto word : queryWordsTfIdf) {
      if (document.second.contains(word.first)) {
        queryDocumentMultipSum += word.second * document.second.at(word.first);
      }
    }

    double documentMagnitude = tfIdfIndex->documents_magnitudes()
                                   ->LookupByKey(document.first)
                                   ->second();

    double cosineSimilarity =
        (queryDocumentMultipSum) / (queryMagnitude * documentMagnitude);

    searchResults.insert({document.first, cosineSimilarity});
  }

  return searchResults;
}