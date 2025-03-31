The assginment contains three parts: parser, indexer, and search engine.

To build the code, run 'make' command first.


1. Parser
Source code is in parser.cpp
To run the parser, run './parser <file_name>' command. Example: ./parser ./data/wsj.xml
The parser will output words line by line, with a blank line between documents.


2. Indexer
Source code is in indexer.cpp
To run the indexer, run './indexer <file_name>' command. Example: ./indexer ./data/wsj.xml
The indexer will create four index files:
    (1) index_words.bin: All the words, along with their posting offsets and the number of documents in their postings
		// Stored as: 4 byte word count + [(wordLength(1 byte), word, pos(4 bytes), docCount(4 bytes)), ...]
		// -- pos: how many documents before the word's first document
		// -- docCount: how many documents the word appears in (vector's size) 

    (2) index_wordPostings.bin: The postings file, including document ids(1, 2, 3, ...) and corresponding term frequency
		// Stored as [(docId, tf), ...] each in 4 bytes 
    
    (3) index_docNo.bin: DOCNO file, for showing DOCNO after retrieving docId
        // Stored as [docNo1, docNo2, ...]. String splited by \0. Example: (WSJ870324-0001\0WSJ870323-0181\0...)

    (4) index_docLengths.bin: Document lengths for calculating scores
        // Stored as [docLength1, docLength2, ...] each in 4 bytes


3. Search engine
Source code is in searchEngine.cpp
To run the search engine, run './searchEngine' command, then enter the query and press ENTER.
Or use pipeline to pass query to the searchEngine. Example: echo James Rosenfield | ./searchEngine
The programme will output the DOCNO and relevance scores of all the relevant documents, ranked by their relevance scores.
The relevance score is calculated using Okapi BM25 https://en.wikipedia.org/wiki/Okapi_BM25, with k1=1.2 and b=0.75.
