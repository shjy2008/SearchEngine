# Information Retrieval System

A full-pipeline information retrieval system featuring a document parser, an inverted index builder, and a ranked search engine using **Okapi BM25** scoring.

---

## Overview

This project consists of three components that work together to parse, index, and search a document corpus:

| Component | Source | Binary |
|-----------|--------|--------|
| Parser | `parser.cpp` | `./parser` |
| Indexer | `indexer.cpp` | `./indexer` |
| Search Engine | `searchEngine.cpp` | `./searchEngine` |

---

## Getting Started

Build all components with a single command:

```bash
make
```

---

## Components

### 1. Parser

Parses an XML document corpus and outputs tokenized words, with a blank line separating each document.

**Usage:**
```bash
./parser <file_name>
```

**Example:**
```bash
./parser ./data/wsj.xml
```

**Output:** Words printed line by line, with a blank line between documents.

---

### 2. Indexer

Builds an inverted index from the document corpus. Produces four binary index files used by the search engine.

**Usage:**
```bash
./indexer <file_name>
```

**Example:**
```bash
./indexer ./data/wsj.xml
```

**Output files:**

#### `index_words.bin`
All vocabulary terms with their posting offsets and document frequencies.
```
Format: 4-byte word count + [(wordLength (1 byte), word, pos (4 bytes), docCount (4 bytes)), ...]
```
- `pos` — number of documents before this word's first posting
- `docCount` — number of documents the word appears in

#### `index_wordPostings.bin`
Postings list containing document IDs and term frequencies.
```
Format: [(docId, tf), ...] — each value stored as 4 bytes
```

#### `index_docNo.bin`
Maps internal document IDs to original DOCNO strings.
```
Format: null-terminated strings, e.g. WSJ870324-0001\0WSJ870323-0181\0...
```

#### `index_docLengths.bin`
Document lengths used for BM25 score normalization.
```
Format: [docLength1, docLength2, ...] — each stored as 4 bytes
```

---

### 3. Search Engine

Queries the index and returns ranked results using **Okapi BM25** relevance scoring.

**Usage (interactive):**
```bash
./searchEngine
```
Then type your query and press `ENTER`.

**Usage (pipeline):**
```bash
echo "James Rosenfield" | ./searchEngine
```

**Example output:**
```
James Rosenfield
wsj870324-0001   19.9359
wsj870413-0059   17.5287
wsj871118-0032   17.1951
wsj891009-0161   15.7512
wsj870826-0073   14.4108
wsj880801-0006   13.7800
wsj911206-0111   13.0946
wsj881018-0006   12.5689
wsj900614-0033   11.0270
wsj900507-0068   10.7572
...
```

---

## Ranking Algorithm

Relevance scores are computed using [**Okapi BM25**](https://en.wikipedia.org/wiki/Okapi_BM25) with the following parameters:

| Parameter | Value |
|-----------|-------|
| `k1` | 1.2 |
| `b` | 0.75 |
