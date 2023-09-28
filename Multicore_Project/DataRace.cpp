#include <iostream>
#include <thread>
#include <mutex>
using namespace std;
/*
�ǽ� #11
� �������� ������? �Ѻ��⿡�� DataRace�� ������ ���࿡�� ������ ����δ�.

Debug ��忡�� ���� ���� �������� Release ��忡�� ���� ���� ��, 
Receiver�� ����� �����ϰ� �Է��� ��ٸ���?

while(!bReady);�� ����ȭ�� ������ �ʴ´�..?
�����Ϸ����� while(!bReady);�� �����Ѵ�!

�׷� ��� �ؾ��ϴ°�?
1. lock/unlock�� ����Ѵ�.
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
} << �̷��� �����Ѱ͵� lock/unlock�� ����ϴ°� �ƴ� �� ����..
���� �Ǵ� �޸��� ��� ����ȭ�� ��Ű�� �ʰ� �غ���!

2. volatile�� ����Ѵ�.
>> �ݵ�� �޸𸮸� �а� ���� �ȴ�.
>> ������ �������Ϳ� �Ҵ����� �ʴ´�.
>> �ڵ� ���ġ�� ���� �ʾ� �а� ���� ������ ��Ų��.

volatile bool bReady = false;
volatile int nData = 0;

��Ƽ ������ ���α׷����� �� ��, ��Ȥ���ٰ� �����Ϸ� ���׷� ����
���ϴ� ����� �ȳ��� ���� �ִ�. 

������� �� �� �˾ƾ� ������ �ذ��� �� ���� ���̴�.

��, �����͸� volatile�� ����� ���� ���Ǹ� �ؾ��Ѵ�.

volatile int* a;
*a = 1; << volatile�� ����
a = b; << �����Ϸ��� ����ȭ

int* volatile a;
*a = 1; << �����Ϸ��� ����ȭ
a = b; << volatile�� ����
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