#include <iostream>
#include <array>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include <limits>

using namespace std;
using namespace chrono;

constexpr int MAX_THREADS = 32;
constexpr int NUM_TEST = 1000'0000;

class NODE {
public:
	int key;
	NODE* volatile next;
	NODE() { key = -1; next = NULL; }
	NODE(int key_value) {
		next = NULL;
		key = key_value;
	}
	~NODE() {}

};

class CSTACK {
	NODE* volatile top;
	mutex top_l;
public:
	CSTACK()
	{
		top = nullptr;
	}
	~CSTACK() { }
	void PUSH(int x)
	{
		NODE* e = new NODE(x);
		top_l.lock();
		e->next = top;
		top = e;
		top_l.unlock();
	}
	int POP()
	{
		top_l.lock();
		if (top == nullptr)
		{
			top_l.unlock();
			return -2;
		}

		int temp = top->key;
		NODE* ptr = top;
		top = top->next;
		top_l.unlock();
		delete ptr;
		return temp;
	}

	void PrintTwenty() {
		NODE* p = top;
		int count = 0;
		for (int i = 0; i < 20; ++i)
		{
			if (p == nullptr) break;
			cout << p->key << " ";
			p = p->next;
		}
		cout << endl;
	}
	void Init()
	{
		NODE* p = top;
		while (p != nullptr)
		{
			NODE* t = p;
			p = p->next;
			delete t;
		}
		top = nullptr;
	}
	
};
class LFSTACK {
	NODE* volatile top;
public:
	LFSTACK()
	{
		top = nullptr;
	}
	~LFSTACK() { }
	void PUSH(int x)
	{
		NODE* e = new NODE(x);
		
		while (1) {
			e->next = top;
			if (CAS(&top, e->next, e)) {
				return;
			}
		}
	}
	int POP()
	{
		while (1) {
			NODE* ptr = top;

			if (ptr == nullptr)
				return -2;

			if (&top, ptr, ptr->next)
				return ptr->key;
		}
	}

	void PrintTwenty() {
		NODE* p = top;
		int count = 0;
		for (int i = 0; i < 20; ++i)
		{
			if (p == nullptr) break;
			cout << p->key << " ";
			p = p->next;
		}
		cout << endl;
	}
	void Init()
	{
		NODE* p = top;
		while (p != nullptr)
		{
			NODE* t = p;
			p = p->next;
			delete t;
		}
		top = nullptr;
	}
	bool CAS(NODE* volatile* addr, NODE* oldNode, NODE* newNode)
	{
		return atomic_compare_exchange_strong(
			reinterpret_cast<volatile atomic_llong*>(addr),
			reinterpret_cast<long long*>(&oldNode),
			reinterpret_cast<long long>(newNode)
		);
	}
};

thread_local int tl_id;
int Thread_id()
{
	return tl_id;
}

typedef LFSTACK MY_STACK;

void Worker(MY_STACK* myQueue, int threadNum, int threadID)
{
	tl_id = threadID;
	for (int i = 0; i < NUM_TEST / threadNum; i++) {
		if ((rand() % 2) || i < 1024 / threadNum) {
			myQueue->PUSH(i);
		}
		else {
			myQueue->POP();
		}
	}
}

int main()
{
	cout << "======== SPEED CHECK =============\n";

	for (int i = 1; i <= MAX_THREADS; i *= 2)
	{
		MY_STACK myStack;

		vector<thread> threads;
		auto start_t = high_resolution_clock::now();

		for (int j = 0; j < i; ++j)
			threads.emplace_back(Worker, &myStack, i, j);

		for (thread& t : threads)
			t.join();

		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;

		myStack.PrintTwenty();
		myStack.Init();

		printf("THREAD_NUM = %d\n", i);
		printf("EXEC TIME = %lld msec\n", duration_cast<milliseconds>(exec_t).count());
		printf("\n");
	}

	return 0;
}