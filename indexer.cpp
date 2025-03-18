#include <iostream>
#include <fstream>
#include <sstream>

// Extract words from a text string
std::vector<std::string> extractWords(const std::string& text) {
	std::vector<std::string> words;
	std::istringstream stream(text);

	std::string word;
	while (stream >> word) {
		std::string newWord = "";
		for (size_t i = 0; i < word.length(); ++i) {
			if (std::isalpha(word[i]))
				newWord += std::tolower(word[i]);
		}
		if (newWord.length() > 0)
			words.push_back(newWord);
	}

	return words;
}

std::string stripString(const std::string& text) {
	int start = 0;
	int end = text.length() - 1;

	while (start <= end && isspace(text[start])) {
		++start;
	}

	while (start <= end && isspace(text[end])) {
		--end;
	}
	return text.substr(start, end - start + 1);
}

class Indexer {

private:
	std::string fileName;

	// word -> [(docid_1, term frequency), (docid_1, term frequency), ...]
	std::unordered_map<std::string, std::vector<std::pair<int, int> > > wordToPostings; 

public:
	Indexer(std::string fileName) {
		this->fileName = fileName;
	}

	void addWordToPostings(const std::string& word, int docId) {
		std::vector<std::pair<int, int> >& postings = wordToPostings[word];
		if (postings.size() == 0 || postings[postings.size() - 1].first != docId) {
			postings.push_back({docId, 1});
		}
		else {
			postings[postings.size() - 1].second += 1;
		}
	}

	void runIndexer() {
		std::ifstream file(this->fileName);
		std::string line = "";

		bool readingTag = false;
		bool readingContent = false;

		size_t readStartIndex = 0;
		std::string currentTagName = "";
		std::string currentText = "";
		std::string currentDocNo = "";

		int documentIndex = -1; // ++ when encounter <DOC>
		int lineIndex = 0;

		while (getline(file, line)) {

			readStartIndex = 0;

			for (size_t i = 0; i < line.length(); ++i) {

				// Start of a tag
				if (line[i] == '<') {
					if (readingContent) {
						std::string content = line.substr(readStartIndex, i - readStartIndex);
						currentText += content;

						if (currentTagName != "DOCNO") // the '<' of </DOCNO>. (Don't need to extract texts of DOCNO)
						{ 
							// Extract and print all the words
							std::vector<std::string> words = extractWords(currentText);
							for (size_t wordIndex = 0; wordIndex < words.size(); ++wordIndex) {
								std::string word = words[wordIndex];
								// std::cout << word << std::endl; // output each word as a line

								int docId = documentIndex + 1;
								this->addWordToPostings(word, docId);
							}
						}

						// std::cout << content;
						readingContent = false;
					}

					// Start to read tag
					readingTag = true;
					readStartIndex = i + 1;

					continue;
				}

				// End of a tag
				if (line[i] == '>') {
					if (readingTag) {
						std::string tagName = line.substr(readStartIndex, i - readStartIndex);
						if (tagName[0] != '/') { // It's an open tag. e.g. <DOC>
							currentTagName = tagName;
							if (currentTagName == "DOC") { // The '>' of <DOC>, the beginning of a document
								// if (documentIndex % 1000 == 0) 
								// {
								// 	std::cout << documentIndex << " documents processed." << std::endl;
								// }
								++documentIndex;
							}
						}
						else { // It's a close tag. e.g. </DOC>

							if (tagName == "/DOC") { // The '>' of </DOC>, the end of a document
								currentDocNo = "";

								std::cout << std::endl; // Output an blank line between documents
							}
							else if (currentTagName == "DOCNO") { // the '>' of </DOCNO>, the close tag of a document no.
								currentDocNo = stripString(currentText);

								// std::cout << currentDocNo << std::endl;
							}

							currentTagName = "";
							currentText = "";
						}
						// std::cout << tagName;
						readingTag = false;
					}

					// Start to read content
					readingContent = true;
					readStartIndex = i + 1;

					continue;
				}
				// std::cout << line[lineIndex];
			}

			// Add the rest of the line to currentText
			if (readingContent) {
				std::string content = line.substr(readStartIndex, line.length() - readStartIndex);
				// std::cout << content;
				currentText += content + "\n";
			}

			// std::cout << std::endl;

			// std::cout << line << std::endl;
			++lineIndex;

			// TODO: for test
			if (lineIndex >= 50)
				break;
		}

		std::cout << "All " << documentIndex + 1 << " documents processed." << std::endl;

	}
};

int main() {

	// TODO: for test
	// std::string testStr = "abcde abckk";
	// std::vector<std::string> words = extractWords(testStr);
	// std::cout << words[0] << words[1] << std::endl;
	// std::string s = "      s aa d   ";
	// std::cout << "<" << stripString(s) << ">" << std::endl;
	// return 0;

	Indexer indexer("wsj.xml");
	indexer.runIndexer();

	return 0;
}
