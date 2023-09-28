#include <atomic>
#include <memory>
#include <thread>
#include <chrono>
#include <iostream>

/*
Atomic Memory의 경우 int, long, float 같은 경우에는 잘 해준다.
하지만 실제 사용 프로그램에서는 data type 만으로 작성할 수 없다.

queue, stack binary tree, vector, ...
우리는 여러가지 자료구조를 사용해 객체를 컨트롤한다.

일반 자료구조에 Lock을 추가하면 너무 느리다.
STL은 프로그램이 죽거나 이상한 결과가 나온다.
STL + Lock은 너무 느리다.

효율적인 Atomic 자료구조가 필요하다..?
'효율적인'은 무엇인가

None - Blocking 프로그래밍이 필요하다!

Lock이 없는 프로그램?
일단, mutex를 쓰지 않는다.
	- Overhead
	- Critical Section(상호배제, 병렬성 X)
	- Priroity inversion(우선순위 역전)
	- Convoying

Lock만 없으면 되는가? - 아니다.
mutex와 비슷한 프로그램 방식을 쓰는 한, 성능 개선을 얻기 힘들다.
	- while(other_thread.flag == true);

상대방 쓰레드의 행동에 의존하지 않는 구현방식이 필요하다.

Blocking
- 다른 쓰레드의 진행상태에 따라 진행이 막힐 수 있다.
- Lock을 사용하면 Blocking
- 멀티쓰레드의 bottle neck?이 생긴다.

Non-Blocking
- 다른 쓰레드가 어떠한 삽질을 하고 있던 상관없이 진행
- 공유 메모리 읽기/쓰기, Atomic(Interlocked) Operation

Priority Inversion
- Inqueue와 Dequeue 중에 우선순위가 있다.
- 덜 중요한 작업들이 중요한 작업의 실행을 막는다.
- Reader/Write Problem에서 많이 발생

Convoying(초보자들이 많이 하는 실수)
- Lock을 얻은 쓰레드가 스케쥴링에서 제외된 경우, lock을 기다리는 모든 쓰레드가 공회전
- Core보다 많은 수의 thread를 생성했을 경우 자주 발생

Non-Blocking 등급
- 무대기(wait-free)
모든 메소드가 정해진 유한한 단계에 실행을 끝마침
멈춤 없는 프로그램 실행

- 무자금(lock-free)
항상, 적어도 한개의 메소드가 유한한 단계에 실행을 끝마침
무대기면 무잠금이다.
기아상태를 유발하기도 한다(거의 그럴일은 없다).
성능을 위해 무대기 대신 무잠금을 선택하기도 한다.

우리는 lock-free를 많이 사용한다.
*/
struct NUM {
	alignas(64) volatile long long num;
};

const int MAX_TH = 2;

using namespace std;
using namespace chrono;

NUM sum{ 0 };
atomic<int> nVictim = 0;
atomic<bool> bFlag[] = { false, false };

void Lock(int nID)
{
	int other = 1 - nID;
	bFlag[nID] = true;
	nVictim = nID;
	while (bFlag[other] && nVictim == nID) {}
}
void UnLock(int nID)
{
	bFlag[nID] = false;
}
void func(int th_id)
{
	for (int i = 0; i < 25000000; ++i) {
		Lock(th_id);
		sum.num = sum.num + 2;
		UnLock(th_id);
	}
}
int main()
{
	auto startTime = high_resolution_clock::now();
	
	thread t1{ func, 0 };
	thread t2{ func, 1 };
	t1.join();
	t2.join();

	auto endTime = high_resolution_clock::now();
	auto durationTime = endTime - startTime;

	cout << "SUM = " << sum.num << endl;
	cout << "Duration = " << duration_cast<milliseconds>(durationTime).count() << "msec" << endl;
}