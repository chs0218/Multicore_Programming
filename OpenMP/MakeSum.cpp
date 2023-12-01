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

OpenMP는 오버헤드가 크고 Data Dependency를 개발자가 책임져야한다.

Intel Thread Building Block
쓰레드 사용에 편리한 여러 API를 제공한다.
인텔에선 잘 돌아가고 AMD는 공식적으로 보장을 해주진 않는다.
그래도 잘 돌아가긴 한다. 비공식적으로 Android/ARM버전도 존재한다.
최근 OneAPI라는 프로젝트에 통합되었고 C++11과의 연동이 강조되고있다.(특히 람다)

STL과 유사한 형태의 non-blocking container를 제공한다.
다양한 형태의 Mutex를 지원한다.
멀티 쓰레드 환경에서의 효율적인 메모리 할당자를 지원한다.(new, delete)
- 기존의 메모리 할당자를 교체한다.
- Cache 일관성도 고려한다.
- 지금은 굳이 쓸 필요는 없지만 컴파일러를 Visual Studio가 아닌 것을 사용한다면 사용해야한다.


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