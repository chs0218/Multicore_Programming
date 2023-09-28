#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
using namespace std;
/*
C++ 11 라이브러리를 사용하지 않고
최대한 단순하고 가볍게 상호배제를 구현해보자

피터슨, 데커, 빵집 등 여러가지 알고리즘이 있다.

피터슨
- 2개의 쓰레드 사이의 Lock과 Unlock을 구현하는 알고리즘
- 매개 변수로 쓰레드 아이디를 전달 받으며 값은 0과 1이라고 가정

일단 공유 자원을 사용하고자하면 flag를 ture로 바꾸고
flag를 확인해서 flag가 true면 다른 쓰레드가 임계영역에 있구나 하고 
기다리고 flag가 false면 공유되는 자원을 사용한다.
<< 데드락에 빠질 수 있으므로 변수를 두어 양보할 쓰레드를 정하게 한다.

아무리봐도 문제가 없는데 뭐가 문제인가?
Visual Studio가 또 버그을 일으켰나? << 아니다.

게다가 mutex를 쓴 것보다 느리다.
캐시로 왔다 갔다하는 데이터가 늘어나면서 성능이 떨어졌다.
캐시 라인을 다르게 놓아도 빨라지긴해도 그렇게 빨라지진 않는다.

원인은 다음장에서.. CPU에 문제가 있다!!

mutex가 비효율적이다고 해서 효율적은 mutex를 만들겠다?
<< 굉장히 안일한 생각
아는게 많아야 할 수 있지만 아는게 많아도 힘들다.

빵집 알고리즘 - N개의 쓰레드에서 동기화를 구현
구현을 해보도록 해보자

PC에서의 메모리 접근은 Atomic이 아니다
99.995%는 Atomic이지만 0.005%정도는 Atomic이 아니다.
>> CPU 모델에 따라 다름

읽고 쓰는 순서가 바뀔 수 있다..?
실행순서가 바뀌는건 Out of Execution, Pipline 문제이다.

_asm mfence >> 위 명령어와 밑 명령어는 실행 순서가 절대로 바뀌지 않게 해준다.
단 어셈블리 명령은 Visual Studio에서 64bit에서 사용 불가
이를 이용해 읽고쓰는 순서를 절대 바뀌지 않게 해줌

atomic_thread_fence(std::memory_order_seq_cst);를 사용해서 할 수도 있음

왜 메모리 접근이 Aomic하지 않은가?

CPU는 사기를 친다.
Line Based Cache Sharing
Out of order execution
write buffering
사기치지 않으면 실행 속도가 느려진다.

CPU는 프로그램을 순차적으로 실행하는 척만한다.
이를 보조해 주는 특수한 HW가 존재
싱글코어에서는 절대로 들키지 않는다.
*/

volatile int nVictim = 0;
volatile bool bFlag[] = { false, false };
volatile int sum = 0;
void Lock(int nID)
{
	int other = 1 - nID;
	bFlag[nID] = true;
	nVictim = nID;
	//_asm mfence;
	//atomic_thread_fence(memory_order_seq_cst);
	while(bFlag[other] && nVictim == nID){ }
}
void UnLock(int nID)
{
	bFlag[nID] = false;
}
void func(int th_id)
{
	for (int i = 0; i < 25000000; ++i) {
		Lock(th_id);
		sum = sum + 2;
		UnLock(th_id);
	}
}
int main()
{
	thread t1{ func, 0 };
	thread t2{ func, 1 };
	t1.join();
	t2.join();

	cout << "SUM = " << sum << endl;
}