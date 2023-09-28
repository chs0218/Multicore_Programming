#include <iostream>
#include <vector>
#include <chrono>
#include <thread>

using namespace std;
using namespace std::chrono;
/*
��� Wait-free �˰������� ��ȯ�� �� �ִ°�?
Wait-free�� ������ �� ���� �˰��� �ִ°�?
�˰����� Wait-free�� ��ȯ�Ϸ��� ��� �ؾ��ϴ°�?

atomic-memory���� ������.

Wait-free�� �����ϸ鼭 ��� ���� �޸𸮷�
Atomic Memory�� ������ �� �ִ� �˰����� �����ϴ°�?
>> �����ϴ�.
>> ���� ����

Wait-free�� �����ϸ鼭 ���� �̱� ������ �ڷᱸ���� 
Atomic Memory�� ����� ��Ƽ ������ �ڷᱸ���� ��ȯ�ϴ� ���� �����Ѱ�?
>> �Ұ����ϴ�.
>> �� �����ֿ� ����

Wait-free�� �����ϸ鼭 �Ϲ����� �ڷᱸ���� ��Ƽ������ �ڷᱸ���� ��ȯ�Ϸ��� 
Atomic Memory���� ������ �� �ʿ��Ѱ�?
>> CompareAndSet() �����̸� ����ϴ�.
>> ���߿� ����

CompareAndSet()�� �����ΰ�?
>> ����ȭ ������ ����

����ȭ ����?
>> �����޸𸮿� ���� ���� - �б�, ����ó�� �� word�� ���� �����̴�.
>> �����峢���� ����ȭ�� ����ȭ ������ ���ؼ� �̷������.
>> �ٸ� ������� ����ϱ� ���� �⺻ ���
>> CPU�� ��ɾ�(�Ǵ� �� ����)�� ����
>> Wait-free�� �����Ǿ� �־���Ѵ�.
	- �ƴϸ� Non-Blocking �˰��򿡼� ����� �� ����.

���� ����ȭ ����
- Load(wait-free)
- Store(wait-free)
- Lock/UnLock(blocking)
- atomic_thread_fence(wait-free)

Cas(wait-free)�� �߰��� �ʿ��ϴ�.

CAS(Compare And Set)����
CAS(expected, update)
>> �޸��� ���� expected�� Update�� �ٲٰ� true�� ����
>> �޸��� ���� expected�� �ƴϸ� false�� ����
>> wait-free ���� ������ atomic load/store�� ������ �� ����

��� CAS�� �Ϲ����� �ڷᱸ���� ��Ƽ������ �ڷᱸ���� ��ȯ�ϴ°�?
>> �˰����� �ִ�.
>> ������ ��ȿ�����̴�.

�ڷᱸ���� ���߾� lock-free �˰����� ������ �����ؾ��Ѵ�.
>> �ٸ����� ���� �� ���� ������ �������� ���� ������ ��������.
>> �ڽſ��� �� �´� ���� ����� ���� ����.

ACE << Lock ����� ���� ����

Convoying�� �ǽɵǸ� this_thread::yield()�� ����غ���.
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