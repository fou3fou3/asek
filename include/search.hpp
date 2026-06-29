#pragma once

#include "tfIdfIndex_generated.h"
#include <set>
#include <string>

struct SearchResult {
  std::string document;
  double cosineSimilarity;
};

struct CompareSearchResults {
  bool operator()(const SearchResult &a, const SearchResult &b) const {
    return a.cosineSimilarity > b.cosineSimilarity;
  }
};

std::vector<char> load_flatbuffer_from_disk(const std::string &filename);

std::multiset<SearchResult, CompareSearchResults>
search_tfidf(const tfIdfIndex::MainPayload *tfIdfIndex, std::string query);
