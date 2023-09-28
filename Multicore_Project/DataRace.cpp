#include <iostream>
#include <thread>
#include <mutex>
using namespace std;
/*
실습 #11
어떤 문제점이 있을까? 겉보기에는 DataRace가 있지만 실행에는 문제가 없어보인다.

Debug 모드에선 문제 없어 보이지만 Release 모드에서 실행 했을 때, 
Receiver가 출력을 먼저하고 입력을 기다린다?

while(!bReady);가 동기화에 먹히지 않는다..?
컴파일러에서 while(!bReady);를 무시한다!

그럼 어떻게 해야하는가?
1. lock/unlock을 사용한다.
void Receiver()
{
	myLock.lock();
	while (!bReady) {
		myLock.unlock();
		myLock.lock();
	}
	myLock.unlock();
	myLock.lock();
	cout << "Data: " << nData << endl;
	myLock.unlock();
}

void Sender()
{
	myLock.lock();
	cin >> nData;
	myLock.unlock();
	myLock.lock();
	bReady = true;
	myLock.unlock();
} << 이렇게 간단한것도 lock/unlock을 사용하는건 아닌 것 같다..
공유 되는 메모리의 경우 최적화를 시키지 않게 해보자!

2. volatile을 사용한다.
>> 반드시 메모리를 읽고 쓰게 된다.
>> 변수를 레지스터에 할당하지 않는다.
>> 코드 재배치를 하지 않아 읽고 쓰는 순서를 지킨다.

volatile bool bReady = false;
volatile int nData = 0;

멀티 스레딩 프로그래밍을 할 때, 간혹가다가 컴파일러 버그로 인해
원하는 결과가 안나올 수도 있다. 

어셈블리를 할 줄 알아야 문제를 해결할 수 있을 것이다.

단, 포인터를 volatile로 사용할 떄는 주의를 해야한다.

volatile int* a;
*a = 1; << volatile이 적용
a = b; << 컴파일러가 최적화

int* volatile a;
*a = 1; << 컴파일러가 최적화
a = b; << volatile이 적용
*/
volatile bool bReady = false;
volatile int nData = 0;
mutex myLock;
void Receiver()
{
	while (!bReady);
	cout << "Data: " << nData << endl;
}

void Sender()
{
	int nBuffer;
	cin >> nBuffer;
	nData = nBuffer;
	bReady = true;
}

int main()
{
	thread t1 = thread(Sender);
	thread t2 = thread(Receiver);

	t1.join();
	t2.join();
}