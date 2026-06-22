#include <cmath>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <ranges>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>


constexpr const char *DATA_DIR = "data";
constexpr const char *INDEX_DIR = "index";
const std::string TFIDF_INDEX_PATH =
    std::format("{}/tfIdfIndex.json", INDEX_DIR);

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
        &tfIdfOfAllWords,
    std::unordered_map<std::string, double> &documentsMagnitudes) {
  size_t numberOfDocuments = 0;

  if (std::filesystem::exists(dirPath) &&
      std::filesystem::is_directory(dirPath)) {
    for (const auto &entry : std::filesystem::directory_iterator(dirPath)) {
      try {
        if (std::filesystem::exists(entry.path()) &&
            std::filesystem::is_regular_file(entry)) {
          std::unordered_map<std::string, uint32_t> documentWordsFrequencies;
          uint32_t numberOfWordsInDocument;
          double documentSquaredSum = 0;
          numberOfDocuments++;

          std::string fileName = entry.path().filename().string();

          get_words_frequencies_from_document(entry, documentWordsFrequencies,
                                              numberOfWordsInDocument);

          for (auto wordFrequency : documentWordsFrequencies) {
            double tfIdfResult =
                (1.0f * wordFrequency.second) / numberOfWordsInDocument;

            tfIdfOfAllWords[wordFrequency.first][fileName] = tfIdfResult;
            documentSquaredSum += std::pow(tfIdfResult, 2);
          }

          documentsMagnitudes[fileName] = std::sqrt(documentSquaredSum);
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
      tfIdfOfAllWords;
  size_t numberOfDocuments;
  std::unordered_map<std::string, double> documentsMagnitudes;

  try {
    // Adds tfs to the map to be used
    numberOfDocuments = calculate_tf_of_all_words(dirPath, tfIdfOfAllWords,
                                                  documentsMagnitudes);
    // tfIdfOfAllWords is only a map of word -> tf at this point
  } catch (const std::filesystem::filesystem_error &e) {
    std::cerr << "Error: " << e.what() << "\n";
  }

  std::cout << tfIdfOfAllWords.size() << "\n";
  std::cout << numberOfDocuments << "\n";

  // Calculating idfs + multiplying them by all the tfs in the hashmap to make
  // it the full map of tf-idfs
  for (auto &word : tfIdfOfAllWords) {
    double idf = std::log10(1.0f * numberOfDocuments / word.second.size());

    for (auto &document : word.second) {
      document.second = document.second * idf;
    }
  } // @TODO tfIdfOfAllWords should be a map of {word: [{document: tfidf}]}
    // instead of {word: {document:tfidf}}

  std::cout << sizeof(tfIdfOfAllWords);

  nlohmann::json j =
      std::make_tuple(tfIdfOfAllWords, numberOfDocuments, documentsMagnitudes);

  std::ofstream file(TFIDF_INDEX_PATH);
  file << j.dump(4);
  file.close();
}

void index() {
  std::cout << "INDEXING!\n";
  index_tf_idf();
}

auto extract_json_from_file(std::string filePath) {
  std::ifstream file(filePath);

  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file: " + filePath);
  }

  return nlohmann::json::parse(file);
}

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
search_tfidf(const auto &tfIdfOfAllWords, std::string query,
             size_t numberOfDocuments,
             std::unordered_map<std::string, double> documentsMagnitudes) {
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
      querySquaredSum += std::pow(it->second, 2);

      // Compute tf-idf of words in the query
      it->second = ((1.0f * it->second) / numberOfWordsInQuery) *
                   std::log10(1.0f * numberOfDocuments /
                              tfIdfOfAllWords[it->first].size());

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

    double cosineSimilarity =
        (queryDocumentMultipSum) /
        (queryMagnitude * documentsMagnitudes[document.first]);

    searchResults.insert({document.first, cosineSimilarity});
  }

  return searchResults;
}

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
      auto documentsMagnitudes = tfIdfIndex[2];

      std::cout << numberOfDocuments << "\n";

      // argv[3] represnts the search query
      auto searchResults = search_tfidf(tfIdfOfAllWords, argv[3],
                                        numberOfDocuments, documentsMagnitudes);
      for (auto result : searchResults | std::views::reverse) {
        std::cout << "URL: " << result.document << "\n";
        std::cout << "Cos-Similarity: " << result.cosineSimilarity << "\n\n\n";
      }
    }
    return 0;
  }

  return 0;
}
