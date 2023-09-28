#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
using namespace std;
/*
C++ 11 ���̺귯���� ������� �ʰ�
�ִ��� �ܼ��ϰ� ������ ��ȣ������ �����غ���

���ͽ�, ��Ŀ, ���� �� �������� �˰����� �ִ�.

���ͽ�
- 2���� ������ ������ Lock�� Unlock�� �����ϴ� �˰���
- �Ű� ������ ������ ���̵� ���� ������ ���� 0�� 1�̶�� ����

�ϴ� ���� �ڿ��� ����ϰ����ϸ� flag�� ture�� �ٲٰ�
flag�� Ȯ���ؼ� flag�� true�� �ٸ� �����尡 �Ӱ迵���� �ֱ��� �ϰ� 
��ٸ��� flag�� false�� �����Ǵ� �ڿ��� ����Ѵ�.
<< ������� ���� �� �����Ƿ� ������ �ξ� �纸�� �����带 ���ϰ� �Ѵ�.

�ƹ������� ������ ���µ� ���� �����ΰ�?
Visual Studio�� �� ������ �����׳�? << �ƴϴ�.

�Դٰ� mutex�� �� �ͺ��� ������.
ĳ�÷� �Դ� �����ϴ� �����Ͱ� �þ�鼭 ������ ��������.
ĳ�� ������ �ٸ��� ���Ƶ� ���������ص� �׷��� �������� �ʴ´�.

������ �����忡��.. CPU�� ������ �ִ�!!

mutex�� ��ȿ�����̴ٰ� �ؼ� ȿ������ mutex�� ����ڴ�?
<< ������ ������ ����
�ƴ°� ���ƾ� �� �� ������ �ƴ°� ���Ƶ� �����.

���� �˰��� - N���� �����忡�� ����ȭ�� ����
������ �غ����� �غ���

PC������ �޸� ������ Atomic�� �ƴϴ�
99.995%�� Atomic������ 0.005%������ Atomic�� �ƴϴ�.
>> CPU �𵨿� ���� �ٸ�

�а� ���� ������ �ٲ� �� �ִ�..?
��������� �ٲ�°� Out of Execution, Pipline �����̴�.

_asm mfence >> �� ��ɾ�� �� ��ɾ�� ���� ������ ����� �ٲ��� �ʰ� ���ش�.
�� ����� ����� Visual Studio���� 64bit���� ��� �Ұ�
�̸� �̿��� �а��� ������ ���� �ٲ��� �ʰ� ����

atomic_thread_fence(std::memory_order_seq_cst);�� ����ؼ� �� ���� ����

�� �޸� ������ Aomic���� ������?

CPU�� ��⸦ ģ��.
Line Based Cache Sharing
Out of order execution
write buffering
���ġ�� ������ ���� �ӵ��� ��������.

CPU�� ���α׷��� ���������� �����ϴ� ô���Ѵ�.
�̸� ������ �ִ� Ư���� HW�� ����
�̱��ھ���� ����� ��Ű�� �ʴ´�.
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