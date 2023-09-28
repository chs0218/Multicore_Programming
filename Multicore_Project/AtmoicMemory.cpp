#include <atomic>
#include <memory>
#include <thread>
#include <chrono>
#include <iostream>

/*
Atomic Memory�� ��� int, long, float ���� ��쿡�� �� ���ش�.
������ ���� ��� ���α׷������� data type ������ �ۼ��� �� ����.

queue, stack binary tree, vector, ...
�츮�� �������� �ڷᱸ���� ����� ��ü�� ��Ʈ���Ѵ�.

�Ϲ� �ڷᱸ���� Lock�� �߰��ϸ� �ʹ� ������.
STL�� ���α׷��� �װų� �̻��� ����� ���´�.
STL + Lock�� �ʹ� ������.

ȿ������ Atomic �ڷᱸ���� �ʿ��ϴ�..?
'ȿ������'�� �����ΰ�

None - Blocking ���α׷����� �ʿ��ϴ�!

Lock�� ���� ���α׷�?
�ϴ�, mutex�� ���� �ʴ´�.
	- Overhead
	- Critical Section(��ȣ����, ���ļ� X)
	- Priroity inversion(�켱���� ����)
	- Convoying

Lock�� ������ �Ǵ°�? - �ƴϴ�.
mutex�� ����� ���α׷� ����� ���� ��, ���� ������ ��� �����.
	- while(other_thread.flag == true);

���� �������� �ൿ�� �������� �ʴ� ��������� �ʿ��ϴ�.

Blocking
- �ٸ� �������� ������¿� ���� ������ ���� �� �ִ�.
- Lock�� ����ϸ� Blocking
- ��Ƽ�������� bottle neck?�� �����.

Non-Blocking
- �ٸ� �����尡 ��� ������ �ϰ� �ִ� ������� ����
- ���� �޸� �б�/����, Atomic(Interlocked) Operation

Priority Inversion
- Inqueue�� Dequeue �߿� �켱������ �ִ�.
- �� �߿��� �۾����� �߿��� �۾��� ������ ���´�.
- Reader/Write Problem���� ���� �߻�

Convoying(�ʺ��ڵ��� ���� �ϴ� �Ǽ�)
- Lock�� ���� �����尡 �����층���� ���ܵ� ���, lock�� ��ٸ��� ��� �����尡 ��ȸ��
- Core���� ���� ���� thread�� �������� ��� ���� �߻�

Non-Blocking ���
- �����(wait-free)
��� �޼ҵ尡 ������ ������ �ܰ迡 ������ ����ħ
���� ���� ���α׷� ����

- ���ڱ�(lock-free)
�׻�, ��� �Ѱ��� �޼ҵ尡 ������ �ܰ迡 ������ ����ħ
������ ������̴�.
��ƻ��¸� �����ϱ⵵ �Ѵ�(���� �׷����� ����).
������ ���� ����� ��� ������� �����ϱ⵵ �Ѵ�.

�츮�� lock-free�� ���� ����Ѵ�.
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