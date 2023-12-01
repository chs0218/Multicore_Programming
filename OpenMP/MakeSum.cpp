#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>
#include <omp.h>

/*
Single Thread: 89ms
OpenMP: 7742ms
MultiThread: 1866ms

OpenMP�� ������尡 ũ�� Data Dependency�� �����ڰ� å�������Ѵ�.

Intel Thread Building Block
������ ��뿡 ���� ���� API�� �����Ѵ�.
���ڿ��� �� ���ư��� AMD�� ���������� ������ ������ �ʴ´�.
�׷��� �� ���ư��� �Ѵ�. ����������� Android/ARM������ �����Ѵ�.
�ֱ� OneAPI��� ������Ʈ�� ���յǾ��� C++11���� ������ �����ǰ��ִ�.(Ư�� ����)

STL�� ������ ������ non-blocking container�� �����Ѵ�.
�پ��� ������ Mutex�� �����Ѵ�.
��Ƽ ������ ȯ�濡���� ȿ������ �޸� �Ҵ��ڸ� �����Ѵ�.(new, delete)
- ������ �޸� �Ҵ��ڸ� ��ü�Ѵ�.
- Cache �ϰ����� ����Ѵ�.
- ������ ���� �� �ʿ�� ������ �����Ϸ��� Visual Studio�� �ƴ� ���� ����Ѵٸ� ����ؾ��Ѵ�.


*/
using namespace std;
using namespace std::chrono;

#define chunk 500'0000
constexpr int MAX_THREAD = 8;

int main()
{
	volatile int sum = 0;
	
	auto start_t = high_resolution_clock::now();

#pragma omp parallel shared(sum)
	{
#pragma omp for schedule (dynamic, chunk) nowait
		for (int i = 0; i < 5000'0000; ++i)
#pragma omp critical
			sum = sum + 2;
	}

//#pragma omp parallel shared(sum)
//	{
//		for (int i = 0; i < 5000'0000 / omp_get_num_threads(); ++i)
//#pragma omp critical
//			sum = sum + 2;
//	}

	auto end_t = high_resolution_clock::now();
	auto exec_t = end_t - start_t;
	int ms = duration_cast<milliseconds>(exec_t).count();

	cout << "OpenMP SUM = " << sum << "\n";
	cout << "OpenMP EXEC = " << ms << " ms\n";


	sum = 0;
	mutex ll;
	start_t = high_resolution_clock::now();

	vector<thread> threads;
	for(int i = 0; i < MAX_THREAD; ++i)
	{
		threads.emplace_back([&sum, &ll]() {
				for (int i = 0; i < 5000'0000 / MAX_THREAD; ++i)
				{
					ll.lock();
					sum = sum + 2;
					ll.unlock();
				}
			}
		);
	}

	for (auto& th : threads)
		th.join();
	
	end_t = high_resolution_clock::now();
	exec_t = end_t - start_t;
	ms = duration_cast<milliseconds>(exec_t).count();

	cout << "Multi Thread SUM = " << sum << "\n";
	cout << "Multli Thread EXEC = " << ms << " ms\n";
}