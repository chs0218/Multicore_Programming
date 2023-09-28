#include <iostream>
#include <atomic>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
using namespace std;
using namespace std::chrono;

constexpr int MAX_THREADS = 32;

struct NUM {
	alignas(64) volatile long long num;
};

NUM sum[32];
mutex myLock;

void ThreadFunc(int th_ID, int threads_num)
{
	for (int i = 0; i < 5000'0000 / threads_num; ++i) {
		sum[th_ID].num += 2;
	}
}

int main()
{
	//char c = getchar();

	for (int i = 1; i <= MAX_THREADS; i *= 2)
	{
		for (auto& s : sum) s.num = 0;

		vector<thread> threads;
		auto start_t = high_resolution_clock::now();
		
		for (int j = 0; j < i; ++j)
			threads.emplace_back(ThreadFunc, j, i);
		
		for (thread& t : threads)
			t.join();

		long long result = 0;
		for (auto& s : sum) result += s.num;

		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		
		printf("THREAD_NUM = %d\n", i);
		printf("SUM = %lld\n", result);
		printf("EXEC TIME = %lld msec\n\n", duration_cast<milliseconds>(exec_t).count());
	}
}