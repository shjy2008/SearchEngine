#include <iostream>
#include <fstream>
#include <sstream>

// Extract words from a text string
std::vector<std::string> extractWords(const std::string& text) {
	std::vector<std::string> words;

	std::string word;
	for (size_t i = 0; i < text.size(); ++i){
		if (std::isalpha(text[i]))
			word += std::tolower(text[i]);
		else if (std::isdigit(text[i]))
			word += text[i];
		else if (text[i] == '-') { // Some words have '-', such as "well-being"
			if (i > 0 && std::isalnum(text[i - 1])) { // Ignore words starting with '-'
				word += text[i];
			}
		}
		else {
			if (word.length() > 0) {
				if (word.length() > 255) { // Length is stored in uint8_t, so truncate the word if its length > 255
					word = word.substr(0, 255);
				}
				words.push_back(word);
			}
			word = "";
		}
	}
	if (word.length() > 0)
		words.push_back(word);

	return words;
}

class Parser {

private:
	std::string fileName;

public:
	Parser(std::string fileName) {
		this->fileName = fileName;
	}

	void runParser() {
		std::ifstream file(this->fileName);
		std::string line = "";

		bool readingTag = false;
		bool readingContent = false;

		size_t readStartIndex = 0;
		std::string currentTagName = "";
		std::string currentText = "";
		std::string currentDocNo = "";

		uint32_t documentIndex = 0; // ++ when encounter </DOC>
		uint32_t lineIndex = 0;

		while (getline(file, line)) {

			readStartIndex = 0;

			for (size_t i = 0; i < line.length(); ++i) {

				// Start of a tag
				if (line[i] == '<') {
					if (readingContent) {
						std::string content = line.substr(readStartIndex, i - readStartIndex);
						currentText += content;

						// Extract and print all the words
						std::vector<std::string> words = extractWords(currentText);

						if (currentTagName == "DOCNO") { // the '<' of </DOCNO>, the close tag of a document no.
							currentDocNo = words[0];
							// Save the currentDocNo
						}

						// Output the words
						for (size_t wordIndex = 0; wordIndex < words.size(); ++wordIndex) {
							std::string word = words[wordIndex];
							// std::cout << word << std::endl; // output each word as a line
						}

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
						}
						else { // It's a close tag. e.g. </DOC>

							// Reach the end of a document
							if (tagName == "/DOC") { 
								currentDocNo = "";

								// Output an blank line between documents
								// std::cout << std::endl; 

								if (documentIndex % 1000 == 0) 
								{
									std::cout << documentIndex << " documents processed." << std::endl;
								}
								++documentIndex;
							}

							currentTagName = "";
							currentText = "";
						}
						
						readingTag = false;
					}

					// Start to read content
					readingContent = true;
					readStartIndex = i + 1;

					continue;
				}
			}

			// Add the rest of the line to currentText
			if (readingContent) {
				std::string content = line.substr(readStartIndex, line.length() - readStartIndex);
				currentText += content + "\n";
			}

			++lineIndex;
		}

		// std::cout << "All " << documentIndex << " documents processed." << std::endl;

	}
};

int main() {
	Parser parser("wsj.xml");
	parser.runParser();

	return 0;
}
