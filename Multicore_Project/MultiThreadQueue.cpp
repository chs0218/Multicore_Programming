#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <mutex>

/*
ť�� ��Ƽ�����忡�� ���� ���� ���̴� �ڷᱸ���̴�.
�츮�� �˾ƾ� �� �κ��� ������ �ʴ�.
Queue�� Stack�� Contains()�޼ҵ带 �������� �ʴ´�.

�޼ҵ��� �������� ����, �κ���, ������ ������ �ִ�.
- ����: Ư�� ������ ��ٸ� �ʿ䰡 ���� ��
	- ����ִ� Ǯ���� get() �� �� ���� �ڵ带 ��ȯ
- �κ���: Ư�� ������ ������ ��ٸ� ��(Non-blocking�� �ƴϴ�.)
	- ����ִ� Ǯ���� get()�� ��, �ٸ� �������� Set() �� �� ���� ��ٸ�
- ������: �ٸ� �������� �޼ҵ� ȣ���� ��ø�� �ʿ�� �� ��
	- �����ζ�� �Ѵ�.
	- �ǿܷ� ������ ����.

������ ����� Lock-Free Queue
- �� ���װ� ����°�?

Data Rce Compiler ����ȭ, Out of order execution, Cache Line Boundary, ABA

ABA ����?
�޸� �������� ���� CAS ���� ���� �߻�

�ذ�å
- �����͸� ������ + �������� Ȯ��, ������ ���� ���� ���� ������ ǥ���Ѵ�.
	- 32��Ʈ�� ��� 32 bit ������, 32 bit �������� ����� 64 bit CAS�� �ذ�
	- 64��Ʈ�� ��� 128 bit CAS�� �ϰų� ������� �ʴ� 16 bit + 2 bit�� �������� �������

- LL, SC ����� ���
	- ���� ���� ���θ� �˻��ϴ� ���
	- ��, Wait-free�� ���� �Ұ���

- shared_ptr�� ����Ѵ�.(������ lock free�� �ƴϴ�.)
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