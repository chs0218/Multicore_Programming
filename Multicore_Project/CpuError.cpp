#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
using namespace std;
/*
*/
const auto SIZE = 5000'0000;
//volatile int x, y;
atomic<int> x, y;
int trace_x[SIZE], trace_y[SIZE];

void ThreadFunc0()
{
	for (int i = 0; i < SIZE; ++i) {
		x = i;
		//atomic_thread_fence(memory_order_seq_cst);
		trace_y[i] = y;
	}
}

void ThreadFunc1()
{
	for (int i = 0; i < SIZE; ++i) {
		y = i;
		//atomic_thread_fence(memory_order_seq_cst);
		trace_x[i] = x;
	}
}

/*
nError가 발생하는 곳에 중단점
v가 -1, 0인데 에러가 난다?
출력해보면 -65536, 65536이 나온다.
>> Cache 문제

캐시는 메모리 복사값을 라인단위로 저장함


*/
volatile bool done = false;
volatile int* bound;
int nError;

void ThreadFunc2()
{
	for (int i = 0; i < 2500'000; ++i) *bound = -(1 + *bound);
	done = true;
}

void ThreadFunc3()
{
	while (!done) {
		int v = *bound;
		if ((v != 0) && (v != -1)) {
			cout << v << ", ";
			nError++;
		}
	}
}

int main()
{
	/*thread t1 = thread{ ThreadFunc0 };
	thread t2 = thread{ ThreadFunc1 };
	t1.join();
	t2.join();

	int count = 0;
	for (int i = 0; i < SIZE; ++i)
	{
		if (trace_x[i] == trace_x[i + 1])
		{
			if (trace_y[trace_x[i]] == trace_y[trace_x[i] + 1]) {
				if (trace_y[trace_x[i]] != i) continue;
				++count;
			}
		}
	}
	cout << "Total Memory Inconsistency: " << count << endl;*/

	int num = 0;
	//bound = &num;
	int ARR[32];
	long long temp = (long long)&ARR[16];
	temp = (temp / 64) * 64;
	temp -= 2;
	bound = (int*)temp;
	*bound = 0;

	thread t1 = thread{ ThreadFunc2 };
	thread t2 = thread{ ThreadFunc3 };
	t1.join();
	t2.join();

	cout << "Total Memory Inconsistency: " << nError << endl;
}