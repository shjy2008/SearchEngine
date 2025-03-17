#include <iostream>
#include <fstream>
#include <sstream>

// Extract words from a text string
std::vector<std::string> extractWords(const std::string& text) {
	std::vector<std::string> words;
	std::istringstream stream(text);

	std::string word;
	while (stream >> word) {
		words.push_back(word);
	}

	return words;
}

int main() {
	std::ifstream file("wsj.xml");

	std::string line = "";

	bool readingTag = false;
	bool readingContent = false;

	bool isCurrentTagFinished = false;
	std::string currentTagName = "";

	std::string currentContent = "";

	std::string currentDocNo = "";

	int documentIndex = -1;


	int lineIndex = 0;
	while (getline(file, line)) {

		// readStartIndex = 0;

		for (size_t i = 0; i < line.length(); ++i) {

			// Start of a tag
			if (line[i] == '<') {
				readingTag = true;
				readingContent = false;
			}
			else if (line[i] == '>') { // End of a tag
				readingTag = false;
				readingContent = true;

				if (isCurrentTagFinished) {
					// std::cout << line[i] << " " << currentTagName << " " << currentContent << std::endl;
					if (currentTagName == "DOCNO") {
						// std::cout << "DOCNO: " << currentContent << std::endl;
						currentDocNo = currentContent;
					}
					else if (currentTagName == "DOC") {
						currentDocNo = "";
						++documentIndex;
						std::cout << std::endl; // TODO: output an blank line between documents
						if (documentIndex % 1000 == 0) 
						{
							std::cout << documentIndex + 1 << " documents processed." << std::endl;
						}
					}
					else {
						std::vector<std::string> words = extractWords(currentContent);
						for (size_t wordIndex = 0; wordIndex < words.size(); ++wordIndex) {
							std::string word = words[wordIndex];
							std::cout << word << std::endl; // TODO: output each word as a line
						}
					}
					isCurrentTagFinished = false;
				}

				currentTagName = "";
				currentContent = "";
			}
			else {
				if (readingTag) {
					if (i > 0 && line[i - 1] == '<' && line[i] == '/') {
						isCurrentTagFinished = true;
					}
					else {
						currentTagName += line[i];
					}
				}
				else if (readingContent) {
					currentContent += line[i];
				}
			}
		}

		if (readingContent) {
			currentContent += '\n';
		}

		// std::cout << std::endl;

		++lineIndex;

		// TODO
		// if (lineIndex >= 50)
		// 	break;
	}

	std::cout << "All " << documentIndex + 1 << " documents processed." << std::endl;

	return 0;
}
