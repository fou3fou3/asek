#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <set>
#include <tuple>
#include <cmath>
#include <format>
#include <nlohmann/json.hpp>

constexpr const char *DATA_DIR = "data";
constexpr const char *INDEX_DIR = "index";

void read_words_frequencies_from_buffer(const std::vector<char> &buffer, std::unordered_map<std::string, uint32_t> &wordsFrequencies, uint32_t &numberOfWordsInDocument)
{
	const char *ptr = buffer.data();
	const char *end = ptr + buffer.size();

	size_t wordCount = 0;

	while (ptr < end)
	{
		while (ptr < end && std::isspace(static_cast<unsigned char>(*ptr)))
		{
			ptr++;
		}

		if (ptr == end)
			break;

		const char *wordStart = ptr;
		while (ptr < end && !std::isspace(static_cast<unsigned char>(*ptr)))
		{
			ptr++;
		}
		const char *wordEnd = ptr;

		std::string word(wordStart, wordEnd);

		std::string realWord;

		for (char c : word)
		{
			if (std::isalpha(static_cast<unsigned char>(c)))
				realWord += std::tolower(c);
		}

		if (!(realWord == ""))
		{
			wordsFrequencies[realWord] += 1;
			wordCount++;
		}
	}

	numberOfWordsInDocument = wordCount;
}

void get_words_frequencies_from_document(const std::filesystem::directory_entry &entry, std::unordered_map<std::string, uint32_t> &wordsFrequencies, uint32_t &numberOfWordsInDocument)
{
	std::ifstream document(entry.path(), std::ios::binary | std::ios::ate);

	if (!document.is_open())
	{
		std::cerr << "Error: Could not open the file!" << std::endl;
	}

	std::streamsize size = document.tellg();
	document.seekg(0, std::ios::beg);

	std::vector<char> buffer(size);
	if (document.read(buffer.data(), size))
	{
		read_words_frequencies_from_buffer(buffer, wordsFrequencies, numberOfWordsInDocument);
	}

	document.close();
}

size_t calculate_tf_of_all_words(const std::filesystem::path &dirPath, std::unordered_map<std::string, std::unordered_map<std::string, double>> &tfIdfOfAllWords)
{
	size_t numberOfDocuments = 0;

	if (std::filesystem::exists(dirPath) && std::filesystem::is_directory(dirPath))
	{
		for (const auto &entry : std::filesystem::directory_iterator(dirPath))
		{
			try
			{
				if (std::filesystem::exists(entry.path()) && std::filesystem::is_regular_file(entry))
				{
					std::unordered_map<std::string, uint32_t> documentWordsFrequencies;
					uint32_t numberOfWordsInDocument;
					numberOfDocuments++;

					std::string fileName = entry.path().filename().string();

					get_words_frequencies_from_document(entry, documentWordsFrequencies, numberOfWordsInDocument);

					for (auto wordFrequency : documentWordsFrequencies)
					{
						tfIdfOfAllWords[wordFrequency.first][fileName] = (1.0f * wordFrequency.second) / numberOfWordsInDocument;
					}
				}
			}
			catch (const std::filesystem::filesystem_error &e)
			{
				std::cerr << "Error: " << e.what() << "\n";
			}

			std::cout << entry.path() << "\n";
		}
	}

	return numberOfDocuments;
}

void index_tf_idf()
{
	std::filesystem::path dirPath = DATA_DIR;
	std::unordered_map<std::string, std::unordered_map<std::string, double>> tfIdfOfAllWords;
	size_t numberOfDocuments;

	try
	{
		// Adds tfs to the map to be used
		numberOfDocuments = calculate_tf_of_all_words(dirPath, tfIdfOfAllWords);
		// tfIdfOfAllWords is only a map of word -> tf at this point
	}
	catch (const std::filesystem::filesystem_error &e)
	{
		std::cerr << "Error: " << e.what() << "\n";
	}

	std::cout << tfIdfOfAllWords.size() << "\n";
	std::cout << numberOfDocuments << "\n";

	// Calculating idfs + multiplying them by all the tfs in the hashmap to make it the full map of tf-idfs
	for (auto &word : tfIdfOfAllWords)
	{
		double idf = std::log10(1.0f * numberOfDocuments / word.second.size());

		for (auto &document : word.second)
		{
			document.second = document.second * idf;
		}
	} // @TODO make a new object where documents are sorted by tfidf

	std::cout << sizeof(tfIdfOfAllWords);

	nlohmann::json j = tfIdfOfAllWords;

	std::ofstream file(std::format("{}/tfIdfIndex.json", INDEX_DIR));
	file << j.dump(4);
	file.close();
}

void index()
{
	std::cout << "INDEXING!\n";
	index_tf_idf();
}

int main(int argc, char *argv[])
{

	if (argc == 1)
	{
		std::cout << "Select a mode (index, search)!\n";
		return 1;
	}

	std::string first_arg = argv[1];

	if (first_arg == "index")
	{
		index();
	}

	return 0;
}
