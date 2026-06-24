
#include "globals.hpp"
#include "index.hpp"
#include "search.hpp"
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

int main(int argc, char *argv[]) {

  if (argc == 1) {
    std::cout << "Select a mode (index, search)!\n";
    return 1;
  }

  std::string first_argument = argv[1];

  if (first_argument == "index") {
    index();
    return 0;
  }

  if (first_argument == "search") {
    std::string second_argument = argv[2];

    if (second_argument == "tfidf") {
      auto tfIdfIndex = extract_json_from_file(TFIDF_INDEX_PATH);
      auto tfIdfOfAllWords = tfIdfIndex[0];
      auto numberOfDocuments = tfIdfIndex[1];
      auto documentsTfIdfSumSquared = tfIdfIndex[2];

      std::cout << numberOfDocuments << "\n";

      while (true) {
        std::string query;

        std::cout << "Enter search query: ";
        std::cin >> query;
        std::cout << "\n";

        auto searchResults =
            search_tfidf(tfIdfOfAllWords, query, numberOfDocuments,
                         documentsTfIdfSumSquared);

        for (auto result : searchResults | std::views::reverse) {
          std::cout << "URL: " << DATA_DIR << "//" << result.document << "\n";
          std::cout << "Cos-Similarity: " << result.cosineSimilarity
                    << "\n\n\n";
        }
      }
    }
    return 0;
  }

  return 0;
}
