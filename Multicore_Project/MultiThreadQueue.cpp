#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <mutex>

/*
큐는 멀티쓰레드에서 가장 많이 쓰이는 자료구조이다.
우리가 알아야 할 부분이 많지는 않다.
Queue나 Stack은 Contains()메소드를 제공하지 않는다.

메소드의 성질에는 완전, 부분적, 동기적 성질이 있다.
- 완전: 특정 조건을 기다릴 필요가 없을 때
	- 비어있는 풀에서 get() 할 때 실패 코드를 반환
- 부분적: 특정 조건의 만족을 기다릴 때(Non-blocking이 아니다.)
	- 비어있는 풀에서 get()할 때, 다른 누군가가 Set() 할 때 까지 기다림
- 동기적: 다른 스레드의 메소드 호출의 중첩을 필요로 할 때
	- 랑데부라고도 한다.
	- 의외로 성능이 좋다.
*/


using namespace std;
using namespace chrono;

constexpr int MAX_THREADS = 32;
constexpr int NUM_TEST = 1000'0000;

class null_mutex
{
public:
	void lock() {}
	void unlock() {}
};

class NODE {
public:
	int key;
	NODE* volatile next;
	NODE() { key = 1; next = NULL; }
	NODE(int key_value) {
		next = NULL;
		key = key_value;
	}
	~NODE() {}
};

class CQUEUE {
	NODE* head;
	NODE* tail;
	mutex enq_l, deq_l;
public:
	CQUEUE()
	{
		head = tail = new NODE{};
	}
	~CQUEUE() { }
	void ENQ(int x)
	{
		NODE* e = new NODE(x);
		enq_l.lock();
		tail->next = e;
		tail = e;
		enq_l.unlock();
	
	}
	int DEQ()
	{
		deq_l.lock();
		if (head->next == nullptr)
		{
			deq_l.unlock();
			return -1;
		}
		int result = head->next->key;
		NODE* tmp = head;
		head = head->next;
		deq_l.unlock();
		delete tmp;
		return result;
	}
	void Init()
	{
		NODE* p = head->next;
		while (p != nullptr)
		{
			NODE* t = p;
			p = p->next;
			delete t;
		}

		tail = head;
		head->next = nullptr;
	}
	void PrintTwenty() {
		NODE* p = head->next;
		int count = 0;
		for (int i = 0; i < 20; ++i)
		{
			if (p == nullptr) break;
			cout << p->key << " ";
			p = p->next;
		}
		cout << endl;
	}
};

class LFQUEUE {
	NODE* head;
	NODE* tail;
public:
	LFQUEUE()
	{
		head = tail = new NODE{};
	}
	~LFQUEUE() { }

	bool CAS(NODE* targetNode, NODE* oldNode, NODE* newNode)
	{
		return atomic_compare_exchange_strong(
			reinterpret_cast<atomic_llong*>(&targetNode),
			reinterpret_cast<long long*>(&oldNode),
			reinterpret_cast<long long>(newNode)
		);
	}

	void ENQ(int x)
	{
		NODE* e = new NODE(x);
		tail->next = e;
		tail = e;

	}
	int DEQ()
	{
		if (head->next == nullptr)
		{
			return -1;
		}
		int result = head->next->key;
		NODE* tmp = head;
		head = head->next;
		delete tmp;
		return result;
	}
	void Init()
	{
		NODE* p = head->next;
		while (p != nullptr)
		{
			NODE* t = p;
			p = p->next;
			delete t;
		}

		tail = head;
		head->next = nullptr;
	}
	void PrintTwenty() {
		NODE* p = head->next;
		int count = 0;
		for (int i = 0; i < 20; ++i)
		{
			if (p == nullptr) break;
			cout << p->key << " ";
			p = p->next;
		}
		cout << endl;
	}
};

thread_local int tl_id;
int Thread_id()
{
	return tl_id;
}

typedef CQUEUE MY_QUEUE;

void Worker(MY_QUEUE* myQueue, int threadNum, int threadID)
{
	tl_id = threadID;
	for (int i = 0; i < NUM_TEST / threadNum; i++) {
		if ((rand() % 2) || i < 128 / threadNum) {
			myQueue->ENQ(i);
		}
		else {
			myQueue->DEQ();
		}
	}
}

int main()
{
	cout << "======== SPEED CHECK =============\n";

	for (int i = 1; i <= MAX_THREADS; i *= 2)
	{
		MY_QUEUE myQueue;

		vector<thread> threads;
		auto start_t = high_resolution_clock::now();

		for (int j = 0; j < i; ++j)
			threads.emplace_back(Worker, &myQueue, i, j);

		for (thread& t : threads)
			t.join();

		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;

		myQueue.PrintTwenty();
		printf("THREAD_NUM = %d\n", i);
		printf("EXEC TIME = %lld msec\n", duration_cast<milliseconds>(exec_t).count());
		printf("\n");
	}

	return 0;
}