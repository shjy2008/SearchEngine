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

	// std::string testStr = "abcde abckk";
	// std::vector<std::string> words = extractWords(testStr);
	// std::cout << words[0] << words[1] << std::endl;

	bool readingTag = false;
	bool readingContent = false;

	size_t readStartIndex = 0;
	std::string currentTagName = "";

	std::string currentText = "";

	std::string currentDocNo = "";

	int documentIndex = -1;


	int lineIndex = 0;
	while (getline(file, line)) {

		readStartIndex = 0;

		for (size_t i = 0; i < line.length(); ++i) {

			// Start of a tag
			if (line[i] == '<') {
				if (readingContent) {
					std::string content = line.substr(readStartIndex, i - readStartIndex);
					currentText += content;

					if (currentTagName != "DOCNO") { // the '<' of </DOCNO>. Don't need to extract texts of DOCNO 
						// Extract and print all the words
						std::vector<std::string> words = extractWords(currentText);
						for (size_t wordIndex = 0; wordIndex < words.size(); ++wordIndex) {
							std::string word = words[wordIndex];
							// std::cout << word << std::endl; // TODO
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
							// std::cout << std::endl; // TODO
							++documentIndex;
						}
					}
					else { // It's an close tag. e.g. </DOC>

						if (tagName == "/DOC") { // The '>' of </DOC>, the end of a document
							currentDocNo = "";

							if (documentIndex % 1000 == 0) 
							{
								std::cout << documentIndex + 1 << " documents processed." << std::endl;
							}
						}
						else if (currentTagName == "DOCNO") { // the '>' of </DOCNO>, the close tag of a document no.
							currentDocNo = currentText;

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

		// TODO
		// if (lineIndex >= 2500)
		// 	break;
	}
	return 0;
}
