#include <iostream>
#include <fstream>
#include <sstream>

class SearchEngine {

private:
	// word -> (pos, docNum)
	// -- pos: how many documents before the word's first document
	// -- docNum: how many documents the word appears in
	std::unordered_map<std::string, std::pair<int, int> > wordPostingsIndex;

public:
	SearchEngine() {
	}

	// Get document length with docId: 1, 2, 3, ... (read from the postings)
	int getDocumentLength(int docId) {
		std::ifstream docLengthsFile("index_docLengths.bin");
		docLengthsFile.seekg(sizeof(int) * (docId - 1), std::ifstream::beg);
		int length = 0;
		docLengthsFile.read((char*)&length, sizeof(length));
		return length;
	}

	// Get document DOCNO with docId
	std::string getDocumentDocNo(int docId) {
		// Seek and read the pos and len first
		std::ifstream docNoIndexFile("index_docNoIndex.bin");
		docNoIndexFile.seekg(sizeof(int) * (docId - 1) * 2, std::ifstream::beg); // * 2 because each doc has pos and len
		int pos = 0;
		int len = 0;
		docNoIndexFile.read((char*)&pos, sizeof(pos));
		docNoIndexFile.read((char*)&len, sizeof(len));

		// Use the pos and len, seek to the specific pos, then read
		std::ifstream docNoFile("index_docNo.bin");
		docNoFile.seekg(pos, std::ifstream::beg);
		std::string docNo(len, '\0');
		docNoFile.read(&docNo[0], len);

		return docNo;
	}

	// Load word postings index
	void loadWordPostingsIndex() {
		std::ifstream wordPostingsIndexFile("index_wordPostingsIndex.bin");
		while (true) {
			uint8_t wordLength = 0;
			wordPostingsIndexFile.read((char*)&wordLength, sizeof(wordLength));

			if (wordPostingsIndexFile.gcount() == 0)
				break;

			std::string word(wordLength, '\0');
			wordPostingsIndexFile.read(&word[0], wordLength);

			int pos = 0;
			wordPostingsIndexFile.read((char*)&pos, sizeof(pos));

			int docNum = 0;
			wordPostingsIndexFile.read((char*)&docNum, sizeof(docNum));

			this->wordPostingsIndex[word] = std::pair<int, int>(pos, docNum);
		}
	}

	// Get word postings
	std::vector<std::pair<int, int> > getWordPostings(const std::string& word) {
		std::vector<std::pair<int, int> > postings;

		std::unordered_map<std::string, std::pair<int, int> >::iterator postingsIndexIt = this->wordPostingsIndex.find(word);
		if (postingsIndexIt == this->wordPostingsIndex.end()) {
			return postings; // Can't find the word, return empty vector
		}

		std::pair<int, int> postingsIndexPair = postingsIndexIt->second;
		int pos = postingsIndexPair.first;
		int docNum = postingsIndexPair.second;

		// Seek and read wordPostings.bin to find the postings(docId and tf) of this word
		std::ifstream wordPostingsFile("index_wordPostings.bin");
		wordPostingsFile.seekg(sizeof(int) * pos * 2, std::ifstream::beg); // * 2 because every doc has docId and term frequency
		for (int i = 0; i < docNum; ++i) {
			int docId = 0;
			int tf = 0;
			wordPostingsFile.read((char*)&docId, sizeof(docId));
			wordPostingsFile.read((char*)&tf, sizeof(tf));

			postings.push_back(std::pair<int, int>(docId, tf));
		}

		return postings;
	}



	void run() {
		this->loadWordPostingsIndex();

		std::vector<std::pair<int, int> > a = this->getWordPostings("john");

		int b = 0;

		// std::cout << this->getWordPostings("john") << std::endl;

		// std::cout << this->getWordPostings("is") << std::endl;

		// std::cout << this->getWordPostings("agreement") << std::endl;


		// std::cout << getDocumentDocNo(2) << std::endl;

		// std::cout << getDocumentDocNo(3) << std::endl;

		// std::cout << getDocumentDocNo(4) << std::endl;

		// std::cout << getDocumentDocNo(5) << std::endl;
	}
};

int main() {

	SearchEngine engine;
	engine.run();

	return 0;
}
