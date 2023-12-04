/*
도구 - Nuget 패키지 관리자 - 솔류션용 Nuget 패키지 관리 - intelltbb 설치

concurrent_unordered_map
- insert(), find(), count(), size(), at()이 thread_safe하다.
- erase()가 thread_safe하지 않다.
erase를 넣었을 때, 성능이 좋지 않기 때문에..
따라서 데이터를 넣었다 뺐다 하는 용도로는 사용할 수 없다.

// 11일 종강, 15일 기말고사

tbb에 있는 건 우리가 만드는 것보다 대부분 좋다.
그러나 가끔 thread_safe 하지 않은 메소드가 있기 때문에 잘 알아보고 써야한다.
STL 사용법과 미묘하게 다르다.

크기 증가 감소 때문에 vector는 thread_safe하지 않다.
vector는 resize 가능한 배열이다.
vector의 메모리 접근 때문에 멀티쓰레드에서 사용할 수 없다.

concurrent_vector의 경우 shrink는 불가능하다.
push_back(), grow_by() 등의 메소드를 제공한다.
clear(), swap(), resize(), reserve()는 병렬수행할 수 없다.
원소들이 연속된 주소에 있지 않다.
중간에 장애물이 있으면 넘어서 allocation 한다.
원소를 읽을 때, Data Dependency 고려를 해야한다.
읽을 때, 생성 중일 수 있다.

concurrent_queue의 경우
try_pop(), push()메소드를 제공한다.
empty() 호출이 pop의 성공을 보장하지 않기 때문에 try_pop() 메소드를 제공한다.
unsafe_size(), empty(), clear(), swap()은 thread_safe하지 않다.

RwLock
ReadOnly 메소드는 상호배제 없이 실행한다.
Write 메소드들은 Write Lock을 걸고 Read 메소드들은 Read Lock을 건다.
- search, size, empty 등 << Read 메소드
- Read Lock 끼리는 동시에 실행되게 한다.

그냥 Locking보다는 빨라진다.
Set이나 Map 같은 Read의 비중이 높은 컨테이너에서 사용하기도 한다.
std::shared_lock

Scalable Lock: busy waiting을 없애 CPU 낭비를 막는다. 그러나 운영체제 호출로 인해 overhaed가 크다.
Fair Lock: 임계영역에 도착한 순서대로 lock을 얻는다.
Recursive: 같은 쓰레드는 lock을 다중으로 얻을 수 있다.
Long Wait의 경우 yield 혹은 block


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