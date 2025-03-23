# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -Wall -Wextra -O3 -std=c++11

all: parser indexer searchEngine

parser: parser.cpp
	$(CXX) $(CXXFLAGS) -o parser parser.cpp

indexer: indexer.cpp
	$(CXX) $(CXXFLAGS) -o indexer indexer.cpp

searchEngine: searchEngine.cpp
	$(CXX) $(CXXFLAGS) -o searchEngine searchEngine.cpp