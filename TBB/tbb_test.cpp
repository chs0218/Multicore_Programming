/*
���� - Nuget ��Ű�� ������ - �ַ��ǿ� Nuget ��Ű�� ���� - intelltbb ��ġ

concurrent_unordered_map
- insert(), find(), count(), size(), at()�� thread_safe�ϴ�.
- erase()�� thread_safe���� �ʴ�.
erase�� �־��� ��, ������ ���� �ʱ� ������..
���� �����͸� �־��� ���� �ϴ� �뵵�δ� ����� �� ����.
*/

#include <tbb/parallel_for.h>
#include <iostream>
#include <chrono>
#include <vector>
#include <thread>

using namespace std;
using namespace tbb;
using namespace std::chrono;

int main()
{
	{
		atomic_int sum = 0;
		auto start_t = system_clock::now();
		for (int i = 0; i < 5000'0000; ++i)
			sum += 2;
		auto end_t = system_clock::now();
		auto exec_t = end_t - start_t;
		auto ms = duration_cast<milliseconds>(exec_t).count();

		cout << "Single Thread Sum = " << sum << "\n";
		cout << "Exec: " << ms << "ms\n";
	}
	{
		const int num_threads = thread::hardware_concurrency();
		atomic_int sum = 0;

		vector<thread> threads;
		auto start_t = system_clock::now();

		for (int i = 0; i < num_threads; ++i)
		{
			threads.emplace_back(thread{ [&sum, num_threads]() {
				for (int i = 0; i < 5000'0000 / num_threads; ++i)
					sum += 2;
				} });
		}

		for (auto& th : threads) th.join();
		auto end_t = system_clock::now();
		auto exec_t = end_t - start_t;
		auto ms = duration_cast<milliseconds>(exec_t).count();

		cout << "Multi Thread Sum = " << sum << "\n";
		cout << "Exec: " << ms << "ms\n";
	}
	{
		atomic_int sum = 0;
		auto start_t = system_clock::now();

		parallel_for(0, 5000'0000, [&](int i) { sum += 2; });

		auto end_t = system_clock::now();
		auto exec_t = end_t - start_t;
		auto ms = duration_cast<milliseconds>(exec_t).count();

		cout << "TBB Sum = " << sum << "\n";
		cout << "Exec: " << ms << "ms\n";
	}
}