#include "search.hpp"
#include <cmath>
#include <cstdint>
#include <fstream>
#include <nlohmann/json.hpp>
#include <set>
#include <string>
#include <unordered_map>

// If this is to be used in the future move it elsewhere.
nlohmann::basic_json<> extract_json_from_file(std::string filePath) {
  std::ifstream file(filePath);

  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file: " + filePath);
  }

  return nlohmann::json::parse(file);
}

std::multiset<SearchResult, CompareSearchResults>
search_tfidf(const nlohmann::basic_json<> &tfIdfOfAllWords, std::string query,
             size_t numberOfDocuments,
             std::unordered_map<std::string, double> documentsTfIdfSumSquared) {
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
      documentsQueryWordsTfIdf; // An inverted tfIdfOfAllWords with documents as
                                // keys and only the query words

  for (auto it = queryWordsTfIdf.begin(); it != queryWordsTfIdf.end();) {

    // Check if the word exists in our index
    if (tfIdfOfAllWords.contains(it->first)) {
      // Compute tf-idf of words in the query
      it->second = ((1.0f * it->second) / numberOfWordsInQuery) *
                   std::log10(1.0f * numberOfDocuments /
                              tfIdfOfAllWords[it->first].size());

      querySquaredSum += std::pow(it->second, 2);

      // Fill documentsQueryWordsTfIdf
      for (auto &[document, tfidf] : tfIdfOfAllWords[it->first].items()) {
        documentsQueryWordsTfIdf[document][it->first] = tfidf;
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

    // Remove this after finishing flatbuffers for search because they are
    // precalculated
    double documentMagnitude =
        std::sqrt(documentsTfIdfSumSquared[document.first]);

    double cosineSimilarity =
        (queryDocumentMultipSum) / (queryMagnitude * documentMagnitude);

    searchResults.insert({document.first, cosineSimilarity});
  }

  return searchResults;
}