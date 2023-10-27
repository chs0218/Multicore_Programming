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

무제한 무잠금 Lock-Free Queue
- 왜 버그가 생기는가?

Data Rce Compiler 최적화, Out of order execution, Cache Line Boundary, ABA

ABA 문제?
메모리 재사용으로 인한 CAS 성공 문제 발생

해결책
- 포인터를 포인터 + 스탬프로 확장, 스탬프 값을 변경 시켜 버전을 표시한다.
	- 32비트인 경우 32 bit 포인터, 32 bit 스탬프를 사용해 64 bit CAS로 해결
	- 64비트인 경우 128 bit CAS를 하거나 사용하지 않는 16 bit + 2 bit를 스탬프로 사용하자

- LL, SC 명령의 사용
	- 값의 변경 여부를 검사하는 방법
	- 단, Wait-free로 구현 불가능

- shared_ptr를 사용한다.(하지만 lock free가 아니다.)
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
	NODE* volatile head;
	NODE* volatile tail;
public:
	LFQUEUE()
	{
		head = tail = new NODE{};
	}
	~LFQUEUE() { }

	bool CAS(NODE* volatile* targetNode, NODE* oldNode, NODE* newNode)
	{
		return atomic_compare_exchange_strong(
			reinterpret_cast<volatile atomic_llong*>(targetNode),
			reinterpret_cast<long long*>(&oldNode),
			reinterpret_cast<long long>(newNode)
		);
	}

	void ENQ(int x)
	{
		NODE* e = new NODE(x);
		while (1) {
			NODE* last = tail;
			NODE* next = last->next;
			if (last != tail) continue;
			if (next == nullptr) {
				if (CAS(&(last->next), nullptr, e)) {
					CAS(&tail, last, e);
					return;
				}
			}
			else CAS(&tail, last, next);
		}
	}
	int DEQ()
	{
		while (true) {
			NODE* first = head;
			NODE* last = tail;
			NODE* next = first->next;
			if (first != head) continue;
			if (next == nullptr) {
				return -1;
			}
			int value = next->key;

			if (first == last) {
				CAS(&tail, last, next);
				continue;
			}

			if (CAS(&head, first, next) == false) 
				continue;
			//delete first;
			return value;
		}
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

		head->next = nullptr;
		tail->next = nullptr;
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

class alignas(8) SPTR {
public:
	NODE* volatile ptr;
	int stamp;
public:
	SPTR() : ptr(nullptr), stamp(0) {}
	SPTR(NODE* p, int s) : ptr(p), stamp(s) {}

	NODE* get_next() { return ptr; }
	int get_stamp() { return stamp; }
	const bool operator==(const SPTR rhs) { 
		return rhs.ptr == ptr && rhs.stamp == stamp;
	}
	const bool operator!=(const SPTR rhs) {
		return !(rhs.ptr == ptr && rhs.stamp == stamp);
	}
};
//class SLFQUEUE {
//	SPTR head;
//	SPTR tail;
//	bool CAS(SPTR* targetNode, NODE* oldNode, NODE* newNode, int old_stamp)
//	{
//		SPTR old_ptr{ oldNode, old_stamp };
//		SPTR new_ptr{ newNode, old_stamp + 1 };
//
//		return atomic_compare_exchange_strong(
//			reinterpret_cast<atomic_llong*>(&targetNode),
//			reinterpret_cast<long long*>(&old_ptr),
//			*reinterpret_cast<long long*>(&new_ptr)
//		);
//	}
//public:
//	SLFQUEUE()
//	{
//		head = tail = SPTR(new NODE{}, 0);
//	}
//	~SLFQUEUE() { }
//
//	void ENQ(int x)
//	{
//		NODE* e = new NODE(x);
//		while (1) {
//			NODE* last = tail.ptr;
//			NODE* next = last->next;
//			if (last != tail.ptr) continue;
//			if (next == nullptr) {
//				if (CAS(&(last->next), nullptr, e)) {
//					CAS(&tail, last, e);
//					return;
//				}
//			}
//			else CAS(&tail, last, next);
//		}
//	}
//	int DEQ()
//	{
//		while (true) {
//			SPTR first = head;
//			SPTR last = tail;
//			NODE* next = first.get_next();
//			if (first != head) continue;
//			if (next == nullptr) {
//				return -1;
//			}
//			int value = next->key;
//
//			if (first == last) {
//				CAS(&tail, last.get_next(), next, last.get_stamp());
//				continue;
//			}
//
//			if (CAS(&head, first.get_next(), next, first.get_stamp()) == false)
//				continue;
//
//			delete first.get_next();
//			return value;
//		}
//	}
//	void Init()
//	{
//		NODE* p = head->next;
//		while (p != nullptr)
//		{
//			NODE* t = p;
//			p = p->next;
//			delete t;
//		}
//
//		head->next = nullptr;
//		tail->next = nullptr;
//	}
//	void PrintTwenty() {
//		NODE* p = head->next;
//		int count = 0;
//		for (int i = 0; i < 20; ++i)
//		{
//			if (p == nullptr) break;
//			cout << p->key << " ";
//			p = p->next;
//		}
//		cout << endl;
//	}
//};

thread_local int tl_id;
int Thread_id()
{
	return tl_id;
}

typedef LFQUEUE MY_QUEUE;

void Worker(MY_QUEUE* myQueue, int threadNum, int threadID)
{
	tl_id = threadID;
	for (int i = 0; i < NUM_TEST / threadNum; i++) {
		if ((rand() % 2) || i < 126 / threadNum) {
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
		myQueue.Init();

		printf("THREAD_NUM = %d\n", i);
		printf("EXEC TIME = %lld msec\n", duration_cast<milliseconds>(exec_t).count());
		printf("\n");
	}

	return 0;
}