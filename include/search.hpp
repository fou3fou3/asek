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

std::multiset<SearchResult, CompareSearchResults>
search_tfidf(const tfIdfIndex::MainPayload *tfIdfIndex, std::string query);
