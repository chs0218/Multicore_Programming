#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_hash_map.h>
#include <tbb/concurrent_map.h>
#include <tbb/parallel_for.h>
#include <vector>
#include <string> 
#include <fstream>
#include <iostream>
#include <chrono>
#include <atomic>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <locale>

using namespace tbb;
using namespace std;
using namespace chrono;

// Structure that defines hashing and comparison operations for user's type. 
typedef concurrent_unordered_map<string, int> StringTable;

int main()
{
	vector<string> Data;
	vector<string> Original;
	StringTable table;
	locale loc;

	cout << "Loading!\n";
	ifstream openFile("TextBook.txt");
	if (openFile.is_open()) {
		string word;
		while (false == openFile.eof()) {
			openFile >> word;
			if (isalpha(word[0], loc))
				Original.push_back(word);
		}
		openFile.close();
	}

	cout << "Loaded Total " << Original.size() << " Words.\n";

	cout << "Duplicating!\n";

	for (int i = 0; i < 1000; ++i)
		for (auto& word : Original) Data.push_back(word);

	cout << "Counting!\n";

	{
		auto start = high_resolution_clock::now();
		unordered_map<string, int> table;
		for (auto& w : Data)
			table[w]++;
		auto du = high_resolution_clock::now() - start;

		int count = 0;
		for (auto& w : table) {
			if (count++ > 10) break;
			cout << "[" << w.first << ", " << w.second << "], ";
		}

		cout << "\nSingle Thread Time : " << duration_cast<milliseconds>(du).count() << endl;
	}
	{
		int num_th = thread::hardware_concurrency() / 2;
		vector<thread> threads;
		auto start = high_resolution_clock::now();
		unordered_map<string, int> table;
		mutex tl;
		for (int i = 0; i < num_th; ++i)
			threads.emplace_back([&tl, &Data, &table, i, num_th]() {
			const int task = Data.size() / num_th;
			const int data_size = Data.size();
			for (int j = 0; j < task; ++j) {
				int loc = j + i * task;
				if (loc >= data_size) break;
				tl.lock();
				table[Data[j + i * task]]++;
				tl.unlock();
			}});

		for (auto& th : threads) th.join();

		auto du = high_resolution_clock::now() - start;

		int count = 0;
		for (auto& w : table) {
			if (count++ > 10) break;
			cout << "[" << w.first << ", " << w.second << "], ";
		}

		cout << "\nMulti Thread Time : " << duration_cast<milliseconds>(du).count() << endl;
	}
	{
		auto start = high_resolution_clock::now();
		concurrent_unordered_map<string, atomic_int> table;
		const int DATA_SIZE = Data.size();
		parallel_for(0, DATA_SIZE, [&Data, &table](int i) {
			table[Data[i]]++;
			});

		auto du = high_resolution_clock::now() - start;

		int count = 0;
		for (auto& w : table) {
			if (count++ > 10) break;
			cout << "[" << w.first << ", " << w.second << "], ";
		}

		cout << "\nTBB Time : " << duration_cast<milliseconds>(du).count() << endl;
	}
	{
		auto start = high_resolution_clock::now();
		concurrent_hash_map<string, atomic_int> table;
		const int DATA_SIZE = Data.size();
		parallel_for(0, DATA_SIZE, [&Data, &table](int i) {
			concurrent_hash_map<string, atomic_int>::accessor p;
			table.insert(p, Data[i]);
			p->second++;
			});

		auto du = high_resolution_clock::now() - start;

		int count = 0;
		for (auto& w : table) {
			if (count++ > 10) break;
			cout << "[" << w.first << ", " << w.second << "], ";
		}

		cout << "\nTBB concurrent hash_map Time : " << duration_cast<milliseconds>(du).count() << endl;
	}
	{
		auto start = high_resolution_clock::now();
		concurrent_map<string, atomic_int> table;
		const int DATA_SIZE = Data.size();
		parallel_for(0, DATA_SIZE, [&Data, &table](int i) {
			table[Data[i]]++;
			});

		auto du = high_resolution_clock::now() - start;

		int count = 0;
		for (auto& w : table) {
			if (count++ > 10) break;
			cout << "[" << w.first << ", " << w.second << "], ";
		}

		cout << "\nTBB concurrent map Time : " << duration_cast<milliseconds>(du).count() << endl;
	}

	// Prev Code
	/*auto start = high_resolution_clock::now();

	parallel_for(size_t(0), Data.size(), [&table, &Data](int i) {
		(*reinterpret_cast<atomic_int*>(&table[Data[i]]))++;
		});

	auto du = high_resolution_clock::now() - start;

	int count = 0;
	for (StringTable::iterator i = table.begin(); i != table.end(); ++i) {
		if (count++ > 10) break;
		printf("[%s %d], ", i->first.c_str(), i->second);
	}

	cout << "\nParallel_For Time : " << duration_cast<milliseconds>(du).count() << endl;
	

	unordered_map<string, int> SingleTable;

	start = high_resolution_clock::now();
	for (auto& word : Data) SingleTable[word]++;
	du = high_resolution_clock::now() - start;

	count = 0;
	for (auto& item : SingleTable) {
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
		int chunk = (size / 8) + 1;
		for (int j = (i * chunk); j < ((i + 1) * chunk); j++) {
			if (j >= size) break;
			(*reinterpret_cast<atomic_int*>(&table2[Data[j]]))++;
		};
		});
	for (auto& th : threads) th.join();
	du = high_resolution_clock::now() - start;

	count = 0;
	for (auto& item : table2) {
		if (count++ > 10) break;
		printf("[%s %d],", item.first.c_str(), item.second);
	}

	cout << "\nConcurrent_Unordered_Map Thread Time : " << duration_cast<milliseconds>(du).count() << endl;

	system("pause");*/
}
