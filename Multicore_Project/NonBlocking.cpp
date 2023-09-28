#include <iostream>
#include <vector>
#include <chrono>
#include <thread>

using namespace std;
using namespace std::chrono;
/*
어떻게 Wait-free 알고리즘으로 변환할 수 있는가?
Wait-free로 구현할 수 없는 알고리즘도 있는가?
알고리즘을 Wait-free로 변환하려면 어떻게 해야하는가?

atomic-memory부터 만들어보자.

Wait-free를 유지하면서 어떻게 실제 메모리로
Atomic Memory로 제작할 수 있는 알고리즘이 존재하는가?
>> 가능하다.
>> 교재 참조

Wait-free를 유지하면서 기존 싱글 쓰레드 자료구조도 
Atomic Memory를 사용해 멀티 쓰레드 자료구조로 변환하는 것이 가능한가?
>> 불가능하다.
>> 다 다음주에 증명

Wait-free를 유지하면서 일반적인 자료구조를 멀티쓰레드 자료구조로 변환하려면 
Atomic Memory말고 무엇이 더 필요한가?
>> CompareAndSet() 연산이면 충분하다.
>> 나중에 증명

CompareAndSet()은 무엇인가?
>> 동기화 연산의 일종

동기화 연산?
>> 공유메모리에 대한 연산 - 읽기, 쓰기처럼 한 word에 대한 연산이다.
>> 쓰레드끼리의 동기화는 동기화 연산을 통해서 이루어진다.
>> 다른 쓰레드와 통신하기 위한 기본 기능
>> CPU의 명령어(또는 그 조합)로 구현
>> Wait-free로 구현되어 있어야한다.
	- 아니면 Non-Blocking 알고리즘에서 사용할 수 없다.

기존 동기화 연산
- Load(wait-free)
- Store(wait-free)
- Lock/UnLock(blocking)
- atomic_thread_fence(wait-free)

Cas(wait-free)의 추가가 필요하다.

CAS(Compare And Set)연산
CAS(expected, update)
>> 메모리의 값이 expected면 Update로 바꾸고 true를 리턴
>> 메모리의 값이 expected가 아니면 false를 리턴
>> wait-free 조건 때문에 atomic load/store로 구현할 수 없다

어떻게 CAS로 일반적인 자료구조를 멀티쓰레드 자료구조로 변환하는가?
>> 알고리즘이 있다.
>> 하지만 비효율적이다.

자료구조에 맞추어 lock-free 알고리즘을 일일이 개발해야한다.
>> 다른데서 구해 쓸 수도 있지만 범용적일 수록 성능이 떨어진다.
>> 자신에게 딱 맞는 것을 만드는 것이 좋다.

ACE << Lock 명령을 위해 개발

Convoying이 의심되면 this_thread::yield()를 사용해보자.
*/

const int MAX_THREADS = 16;

volatile int sum;
volatile int LOCK = 0;
bool CAS(volatile int* addr, int expected, int new_val)
{
	return atomic_compare_exchange_strong(
		reinterpret_cast<volatile atomic_int*>(addr), &expected, new_val);
}
void CAS_LOCK()
{
	while (!CAS(&LOCK, 0, 1)) this_thread::yield();
}
void CAS_UNLOCK()
{
	LOCK = 0;
}
void worker(int num_threads)
{
	const int loop_count = 50000000 / num_threads;
	for (auto i = 0; i < loop_count; ++i) {
		CAS_LOCK();
		sum = sum + 2;
		CAS_UNLOCK();
	}
}
int main()
{
	for (int i = 1; i <= MAX_THREADS; i *= 2)
	{
		sum = 0;

		vector<thread> threads;
		auto start_t = high_resolution_clock::now();

		for (int j = 0; j < i; ++j)
			threads.emplace_back(worker, i);

		for (thread& t : threads)
			t.join();

		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;

		printf("THREAD_NUM = %d\n", i);
		printf("SUM = %d\n", sum);
		printf("EXEC TIME = %lld msec\n\n", duration_cast<milliseconds>(exec_t).count());
	}
	return 0;
}