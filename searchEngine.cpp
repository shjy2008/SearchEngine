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
		else if (text[i] == '-') { // some words have '-', such as "well-being"
			if (i > 0 && std::isalnum(text[i - 1])) {
				word += text[i];
			}
		}
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

bool sortScoreCompare(const std::pair<int, float>& a, const std::pair<int, float>& b) {
	return a.second > b.second;
}

class SearchEngine {

private:
	std::ifstream docLengthsFile; // Document lengths for calculating scores. 4 bytes int each document length
	std::ifstream docNoIndexFile; // DOCNO index file, for seeking and reading DOCNO. (pos1, len1, pos2, len2, ...) each in 4 bytes int
	std::ifstream docNoFile; // DOCNO file, for showing DOCNO after retrieving docId
	std::ifstream wordPostingsIndexFile; // Word postings index, for seeking and reading word postings. Stored as (wordLength(int8), word, pos(int), docNum(int))
	std::ifstream wordPostingsFile; // Word postings

	int totalDocuments; // number of documents in total, initialize after loading index_docLengths.bin
	float averageDocumentLength; // Average length of all the documents, used for BM25
	std::unordered_map<int, int> docIdToLength; // docId -> documentLength

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

		// Read docLengths.bin to get total document number and average document length
		int length = 0;
		int docId = 0;
		int totalLength = 0;
		while (docLengthsFile.read((char*)&length, sizeof(length))) {
			++docId;
			this->docIdToLength[docId] = length;
			totalLength += length;
		}
		this->totalDocuments = docId;
		this->averageDocumentLength = (float)totalLength / this->totalDocuments;
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
		std::unordered_map<int, int>::iterator itr = this->docIdToLength.find(docId);
		if (itr != this->docIdToLength.end()) {
			return itr->second;
		}
		return 0;
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

	// tf_td: number of the term appears in doc
	// docLength: how many words in the document
	// idf: inverted document frequency (calculated by total document and documents contain the word)
	float getRankingScore(int tf_td, int docLength, float idf) {
		// TF-IDF
		// float tf_td_normalized = (float)tf_td / docLength;
		// float idf = (float)this->totalDocuments / docNumContainWord;
		// return tf_td_normalized * idf;

		// BM25 - slide (but it will produce negative value when docNumContainWord > 1/2 totalDocuments, and the ranking is wrong)
		// float w_t = log2f((this->totalDocuments - docNumContainWord + 0.5f) / (docNumContainWord + 0.5f));
		// float k1 = 1.2f;
		// float k3 = 7;
		// float b = 0.75f;
		// float K = k1 * ((1 - b) + (b * docLength / this->averageDocumentLength));
		// float w_dt = w_t * ((k1 + 1) * tf_td / (K + tf_td)) * ((k3 + 1) * tf_tq / (k3 + tf_tq));
		// return w_dt;

		// Okapi BM25 https://en.wikipedia.org/wiki/Okapi_BM25
		float k1 = 1.2f;
		float b = 0.75f;
		float K = k1 * ((1 - b) + b * (docLength / this->averageDocumentLength));
		float score = idf * (tf_td * (k1 + 1) / (tf_td + K));
		return score;
	}

	// input: query (multiple words) e.g. italy commercial
	// output: a list of sorted docId and score. e.g. [(1, 2.5), (10, 2.1), ...]
	std::vector<std::pair<int, float> > getSortedRelevantDocuments(const std::string& query) {
		std::vector<std::string> words = extractWords(query);

		std::unordered_map<int, float> mapDocIdScore;
		for (std::vector<std::string>::iterator itrWords = words.begin(); itrWords != words.end(); ++itrWords) {
			std::string word = *itrWords;
			for (int i = 0; i < word.length(); ++i)
				word[i] = std::tolower(word[i]);

			std::vector<std::pair<int, int> > postings = this->getWordPostings(word);
			int docNumContainWord = postings.size();

			// Okapi BM25 https://en.wikipedia.org/wiki/Okapi_BM25
			float idf = log((this->totalDocuments - docNumContainWord + 0.5) / (docNumContainWord + 0.5) + 1); // Ensure positive

			for (int i = 0; i < postings.size(); ++i) {
				int docId = postings[i].first; // docId (1, 2, 3, ...)
				int tf_td = postings[i].second; // term frequency in doc
	
				int docLength = this->getDocumentLength(docId);
	
				float score = this->getRankingScore(tf_td, docLength, idf);

				// Add score to mapDocIdScore
				if (score > 0) {
					std::unordered_map<int, float>::iterator itrMapDocIdScore = mapDocIdScore.find(docId);
					if (itrMapDocIdScore != mapDocIdScore.end()) {
						itrMapDocIdScore->second += score;
					}
					else {
						mapDocIdScore[docId] = score;
					}
				}
			}	
		}

		std::vector<std::pair<int, float> > vecDocIdScore; // docId and score: [(docId1, score1), (docId2, score2), ...]
		for (std::unordered_map<int, float>::iterator itrMapDocIdScore = mapDocIdScore.begin(); itrMapDocIdScore != mapDocIdScore.end(); ++itrMapDocIdScore) {
			vecDocIdScore.push_back(std::pair<int, float>(itrMapDocIdScore->first, itrMapDocIdScore->second));			
		}

		// Sort by score
		std::sort(vecDocIdScore.begin(), vecDocIdScore.end(), sortScoreCompare);

		return vecDocIdScore;
	}


	void run() {

		// std::string query = "rosenfield wall street unilateral representation";
		// std::string query = "rosenfield";
		std::string query;
		while(std::getline(std::cin, query)) 
		{
			std::vector<std::pair<int, float> > vecDocIdScore = this->getSortedRelevantDocuments(query);
	
			// Print the sorted list of docNo and score
			for (int i = 0; i < vecDocIdScore.size(); ++i) {
				std::string docNo = this->getDocNo(vecDocIdScore[i].first);
				float score = vecDocIdScore[i].second;
	
				std::cout << docNo << " " << score << std::endl;
			}
		}
	}
};

int main() {

	// TODO: time (./search < Q > A) < 1s

	std::chrono::steady_clock::time_point time_begin = std::chrono::steady_clock::now();

	//for (int i = 0; i < 1000; ++i)
	{
		SearchEngine engine;
		engine.load();
		engine.run();
		//break;
	}

	std::chrono::steady_clock::time_point time_end = std::chrono::steady_clock::now();

	std::cout << "Time used: " << std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_begin).count() << "ms" << std::endl;

	return 0;
}
