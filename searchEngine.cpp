#include <iostream>
#include <fstream>
#include <sstream>

class SearchEngine {

private:
	std::ifstream docLengthsFile; // Document lengths for calculating scores. 4 bytes int each document length
	std::ifstream docNoIndexFile; // DOCNO index file, for seeking and reading DOCNO. (pos1, len1, pos2, len2, ...) each in 4 bytes int
	std::ifstream docNoFile; // DOCNO file, for showing DOCNO after retrieving docId
	std::ifstream wordPostingsIndexFile; // Word postings index, for seeking and reading word postings. Stored as (wordLength(int8), word, pos(int), docNum(int))
	std::ifstream wordPostingsFile; // Word postings

	int totalDocuments = 0; // number of documents in total, initialize after loading index_docLengths.bin

	// word -> (pos, docNum)
	// -- pos: how many documents before the word's first document
	// -- docNum: how many documents the word appears in
	std::unordered_map<std::string, std::pair<int, int> > wordPostingsIndex;

public:
	SearchEngine() {
	}

	void load() {
		this->loadIndexFiles(); // load all the index files
		this->loadWordPostingsIndex(); // load word postings index from disk

		// Get total document number by dividing the size of docLength.bin by 4 bytes
        docLengthsFile.seekg(0, std::ifstream::end); // seekg() moves the get pointer to the end
		totalDocuments = docLengthsFile.tellg() / sizeof(int); // tellg() get the position of the get pointer
	}

	// Load all the index files from disk
	void loadIndexFiles() {
		docLengthsFile.open("index_docLengths.bin");
		docNoIndexFile.open("index_docNoIndex.bin");
		docNoFile.open("index_docNo.bin");
		wordPostingsIndexFile.open("index_wordPostingsIndex.bin");
		wordPostingsFile.open("index_wordPostings.bin");
	}

	// Get document length(how many words in doc) with docId: 1, 2, 3, ... (read from the postings)
	int getDocumentLength(int docId) {
		docLengthsFile.seekg(sizeof(int) * (docId - 1), std::ifstream::beg);
		int length = 0;
		docLengthsFile.read((char*)&length, sizeof(length));
		return length;
	}

	// Get document DOCNO with docId
	// return: DOCNO  e.g. WSJ870324-0001
	std::string getDocNo(int docId) {
		// Seek and read the pos and len first
		docNoIndexFile.seekg(sizeof(int) * (docId - 1) * 2, std::ifstream::beg); // * 2 because each doc has pos and len
		int pos = 0;
		int len = 0;
		docNoIndexFile.read((char*)&pos, sizeof(pos));
		docNoIndexFile.read((char*)&len, sizeof(len));

		// Use the pos and len, seek to the specific pos, then read
		docNoFile.seekg(pos, std::ifstream::beg);
		std::string docNo(len, '\0');
		docNoFile.read(&docNo[0], len);

		return docNo;
	}

	// Load word postings index
	void loadWordPostingsIndex() {
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

	// Get word postings. input: word
	// return: [(docId1, tf1), (docId2, tf2), ...], e.g. [(2, 3), (3, 6), ...]
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

	// tf_td: term frequency of term in doc
	// docLength: how many words in the document
	// totalDocuments: how many documents in total
	// docNumContainTerm: how many documents contain the term
	float getRankingScore(int tf, int docLength, int docNumContainWord) {
		float tf_td = (float)tf / docLength;
		float idf = (float)this->totalDocuments / docNumContainWord;
		return tf_td * idf;
	}

	void run() {
		std::string word = "STREET";
		for (int i = 0; i < word.length(); ++i)
			word[i] = std::tolower(word[i]);

		std::vector<std::pair<int, float> > vecDocIdScore; // docId and score: [(docId1, score1), (docId2, score2), ...]
		std::vector<std::pair<int, int> > postings = this->getWordPostings(word);
		for (int i = 0; i < postings.size(); ++i) {
			int docId = postings[i].first; // docId (1, 2, 3, ...)
			int tf = postings[i].second; // term frequency in doc

			int docLength = this->getDocumentLength(docId);
			int docNumContainWord = postings.size();

			float score = this->getRankingScore(tf, docLength, docNumContainWord);
			vecDocIdScore.push_back(std::pair<int, float>(docId, score));
		}

		// Sort by score
		std::sort(vecDocIdScore.begin(), vecDocIdScore.end(), [](const std::pair<int, float>& a, const std::pair<int, float>& b) {
			return a.second > b.second;
		});

		// Print the sorted list of docNo and score
		for (int i = 0; i < vecDocIdScore.size(); ++i) {
			std::string docNo = this->getDocNo(vecDocIdScore[i].first);
			float score = vecDocIdScore[i].second;

			std::cout << docNo << " " << score << std::endl;
		}

		// std::cout << this->getWordPostings("john") << std::endl;

		// std::cout << this->getWordPostings("is") << std::endl;

		// std::cout << this->getWordPostings("agreement") << std::endl;


		// std::cout << getDocNo(2) << std::endl;

		// std::cout << getDocNo(3) << std::endl;

		// std::cout << getDocNo(4) << std::endl;

		// std::cout << getDocNo(5) << std::endl;
	}
};

int main() {

	SearchEngine engine;
	engine.load();
	engine.run();

	return 0;
}
