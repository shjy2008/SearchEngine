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
		else if (std::isdigit(text[i]) || text[i] == '-')// some words have '-', such as "well-being"
			word += text[i];
		else {
			if (word.length() > 0) {
				if (word.length() > 255) { // length is stored in uint8_t, so truncate the word if its length > 255
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
	// (docid: 1, 2, 3, ...)
	// e.g. {"aircraft": [(6, 1), ...], "first": [(5, 1), (6, 2), ...], ...}
	std::unordered_map<std::string, std::vector<std::pair<int, int> > > wordToPostings; 

	// DOCNO list 
	// e.g. [WSJ870324-0001, WSJ870323-0181, ...]
	std::vector<std::string> docNoList;

	// Document length list
	// e.g. [159, 64, 48, 30, 106, 129, ...]
	std::vector<int> documentLengthList;

public:
	Indexer(std::string fileName) {
		this->fileName = fileName;
	}

	void saveIndexToFiles() {
		// Save document length list
		std::ofstream docLengthsFile("index_docLengths.bin"); // an int(4 byte) for each document length
		for (size_t i = 0; i < documentLengthList.size(); ++i) {
			docLengthsFile.write((const char*)&documentLengthList[i], sizeof(documentLengthList[i]));
		}

		// Save DOCNO list
		std::ofstream docNoFile("index_docNo.bin"); // docNo("WSJ870324-0001") stored continuously
		// Use docNoIndexFile.bin to seek and read docNo.bin. Stored as (pos1, len1, pos2, len2, ...) each in 4 bytes int
		// "pos" is the offset from the beginning, "len" is the length of the corresponding DOCNO
		std::ofstream docNoIndexFile("index_docNoIndex.bin");
		int currentPos = 0;
		for (size_t i = 0; i < docNoList.size(); ++i) {
			int length = docNoList[i].length();
			docNoFile.write(docNoList[i].c_str(), length);

			docNoIndexFile.write((const char*)&currentPos, sizeof(currentPos));
			docNoIndexFile.write((const char*)&length, sizeof(length));

			currentPos += length;
		}

		// Save wordToPostings
		// Stored as (docId1 for word1, term frequency 1 for word1, docId2 for word1, tf2 for word1, 
		// 				docId1 for word2, tf1 for word2, ...) each in 4 bytes int
		std::ofstream wordPostingsFile("index_wordPostings.bin");
		// Use index_wordPostingsIndex.bin to seek and read wordPostings.bin
		// Stored as (wordLength(int8), word, pos(int), docNum(int))
		// -- pos: how many documents before the word's first document
		// -- docNum: how many documents the word appears in (vector's size) 
		std::ofstream wordPostingsIndexFile("index_wordPostingsIndex.bin");
		int docCounter = 0;
		for (std::unordered_map<std::string, std::vector<std::pair<int, int> > >::iterator it 
					= wordToPostings.begin(); it != wordToPostings.end(); ++it) 
		{
			std::string word = it->first;
			std::vector<std::pair<int, int> > postings = it->second;
			int docNum = postings.size();

			uint8_t wordLength = (uint8_t)word.length();
			wordPostingsIndexFile.write((const char*)&wordLength, sizeof(wordLength));
			wordPostingsIndexFile.write(word.c_str(), wordLength);
			wordPostingsIndexFile.write((const char*)&docCounter, sizeof(docCounter));
			wordPostingsIndexFile.write((const char*)&docNum, sizeof(docNum));

			for (int i = 0; i < docNum; ++i) {
				int docId = postings[i].first;
				int tf = postings[i].second;
				wordPostingsFile.write((const char*)&docId, sizeof(docId));
				wordPostingsFile.write((const char*)&tf, sizeof(tf));

				++docCounter;
			}
		}
	}

	void addWordToPostings(const std::string& word, int docId) {
		// Since all documents are processed one by one, the current document is always the last one in postings.
		// So don't need wordToPostings.find(word), just access the last one
		std::vector<std::pair<int, int> >& postings = wordToPostings[word];
		if (postings.size() == 0 || postings[postings.size() - 1].first != docId) {
			postings.push_back(std::pair<int, int>(docId, 1));
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
		int currentDocumentLength = 0;

		int documentIndex = 0; // ++ when encounter </DOC>
		int lineIndex = 0;

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
							currentDocNo = words[0]; //stripString(currentText);
							// TODO: Save the currentDocNo
							docNoList.push_back(currentDocNo);
						}

						// Output the words, and save to postings
						for (size_t wordIndex = 0; wordIndex < words.size(); ++wordIndex) {
							std::string word = words[wordIndex];
							// std::cout << word << std::endl; // output each word as a line

							int docId = documentIndex + 1;
							this->addWordToPostings(word, docId);
						}

						// Save document length
						currentDocumentLength += words.size();

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

								// Save current document length
								this->documentLengthList.push_back(currentDocumentLength);
								currentDocumentLength = 0;

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

		std::cout << "All " << documentIndex << " documents processed." << std::endl;

		this->saveIndexToFiles();
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
