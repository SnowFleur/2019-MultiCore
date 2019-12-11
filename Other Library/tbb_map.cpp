#include <tbb/tbb.h>
#include <vector>
#include <string> 
#include <fstream>
#include <iostream>
#include <chrono>
#include <atomic>
#include <thread>
#include <unordered_map>

using namespace tbb;
using namespace std;
using namespace chrono;

// Structure that defines hashing and comparison operations for user's type. 
struct MyHashCompare {
	static size_t hash(const string& x) {
		size_t h = 0;
		for (const char* s = x.c_str(); *s; ++s)
			h = (h * 17) ^ *s;
		return h;
	}
	//! True if strings are equal 
	static bool equal(const string& x, const string& y) {
		return x == y;
	}
};
// A concurrent hash table that maps strings to ints. 
typedef concurrent_hash_map<string, int, MyHashCompare> StringTable;
// Function object for counting occurrences of strings. 
struct Tally {
	StringTable& table;
	Tally(StringTable& table_) : table(table_) {}
	void operator()(const blocked_range<vector <string>::iterator> range) const {
		for (auto p = range.begin(); p != range.end(); ++p) {
			StringTable::accessor a;
			table.insert(a, *p);
			a->second += 1;
		}
	}
};	

int main()
{
	vector<string> Data;
	vector<string> Original;
	StringTable table;

	cout << "Loading!\n";
	ifstream openFile("TextBook.txt");
	if (openFile.is_open()) {
		string word;
		while (false == openFile.eof()) {
			openFile >> word;
			Original.push_back(word);
		}
		openFile.close();
	}

	cout << "Loaded Total " << Original.size() << " Words.\n";

	cout << "Duplicating!\n";

	//for (int i = 0; i < 500; ++i)
	//	for (auto &word : Original) Data.push_back(word);

	cout << "Counting!\n"; 

	auto start = high_resolution_clock::now();
	parallel_for(blocked_range<vector <string>::iterator>(Data.begin(), Data.end(), 1000),
		Tally(table));
	auto du = high_resolution_clock::now() - start;

	int count = 0;
	for (StringTable::iterator i = table.begin(); i != table.end(); ++i) {
		if (count++ > 10) break;
		printf("[%s %d], ", i->first.c_str(), i->second);
	}

	cout << "\nParallel_For Time : " << duration_cast<milliseconds>(du).count() << endl;

	unordered_map<string, int> SingleTable;

	start = high_resolution_clock::now();
	for (auto &word : Data) SingleTable[word]++;
	du = high_resolution_clock::now() - start;

	count = 0;
	for (auto &item : SingleTable) {
		if (count++ > 10) break;
		printf("[%s %d],", item.first.c_str(), item.second);
	}

	cout << "\nSingle Thread Time : " << duration_cast<milliseconds>(du).count() << endl;

	StringTable table2;
	start = high_resolution_clock::now();
	vector<thread> threads;
	int size = static_cast<int>(Data.size());
	int chunk = size / 8;
	for (int i = 0; i < 8; ++i) threads.emplace_back([&, i]() {
		int size = static_cast<int>(Data.size());
		int chunk = (size / 8) + 1;
			for (int j = (i * chunk); j < ((i + 1) *chunk); j++) {
				if (j >= size) break;
				StringTable::accessor a;
				table2.insert(a, Data[j]);
				a->second += 1;
			};
		});
	for (auto &th : threads) th.join();
	du = high_resolution_clock::now() - start;

	count = 0;
	for (auto &item : table2) {
		if (count++ > 10) break;
		printf("[%s %d],", item.first.c_str(), item.second);
	}

	cout << "\nConcurrent_Unordered_Map Thread Time : " << duration_cast<milliseconds>(du).count() << endl;

	system("pause");
}
