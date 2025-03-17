#include <iostream>
#include <fstream>
#include <sstream>

void addCharToWord(std::string& word, char c) {
	// if (isalnum(c)) {
	if (std::isalpha(c)) {
		word += std::tolower(c);
	}
}

void outputWord(const std::string& word) {
	if (word.length() > 0)
		std::cout << word << std::endl; // TODO: output each word as a line
}

int main() {
	std::ifstream file("wsj.xml");

	std::string line = "";

	bool isReadingTag = false;
	bool isReadingContent = false;

	bool isReadingDocNo = false;

	bool isInCloseTag = false;
	std::string currentTagName = "";
	std::string currentContent = "";
	std::string currentDocNo = "";

	int documentIndex = -1;

	int lineIndex = 0;
	while (getline(file, line)) {

		for (size_t i = 0; i < line.length(); ++i) {

			// Start of a tag
			if (line[i] == '<') {
				isReadingTag = true;
				isReadingContent = false;
				outputWord(currentContent);
			}
			else if (line[i] == '>') { // End of a tag
				isReadingTag = false;
				isReadingContent = true;

				if (isInCloseTag) {
					if (currentTagName == "DOCNO") {
						// std::cout << "DOCNO: " << currentContent << std::endl; // Output the DOCNO
						currentDocNo = currentContent;
						// outputWord(currentContent);
						isReadingDocNo = false;
					}
					else if (currentTagName == "DOC") {
						currentDocNo = "";
						// std::cout << std::endl; // TODO: output an blank line between documents
						// if (documentIndex % 1000 == 0) 
						// {
						// 	std::cout << documentIndex << " documents processed." << std::endl;
						// }
						++documentIndex;
					}
					isInCloseTag = false;
				}
				else {
					if (currentTagName == "DOCNO")
						isReadingDocNo = true;
				}

				currentTagName = "";
				currentContent = "";
			}
			else {
				if (isReadingTag) {
					if (i > 0 && line[i] == '/' && line[i - 1] == '<') {
						isInCloseTag = true;
					}
					else {
						currentTagName += line[i];
					}
				}
				else if (isReadingContent && !isReadingDocNo) {
					// If encounter not alphabet, or reach the end of a line, output the word
					if ((!std::isalpha(line[i]) || i == line.length() - 1)
						&& currentContent.length() > 0)
					{
						if (i == line.length() - 1) {
							addCharToWord(currentContent, line[i]);
						}
						outputWord(currentContent);
						currentContent = "";
					}
					else{
						addCharToWord(currentContent, line[i]);
					}
				}
			}
		}

		++lineIndex;

		// For debug only
		if (lineIndex >= 50)
			break;
	}

	std::cout << "All " << documentIndex + 1 << " documents processed." << std::endl;

	return 0;
}
