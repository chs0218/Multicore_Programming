#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <mutex>

using namespace std;
using namespace chrono;

constexpr int MAX_THREADS = 32;

class null_mutex {
public:
	void lock() {}
	void unlock() {}
};

class NODE {
public:
	int v;
	NODE* volatile next;
	NODE() : v(-1), next(nullptr) {}
	NODE(int x) : v(x), next(nullptr) {}
};

class CQUEUE {
	NODE* volatile head, * volatile tail;
	null_mutex enq_l, deq_l;
public:
	CQUEUE()
	{
		head = tail = new NODE{};
	}
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
		if (head->next == nullptr) {
			deq_l.unlock();
			return (-1);
		}
		int result = head->next->v;
		NODE* t = head;
		head = head->next;
		deq_l.unlock();
		delete t;
		return result;
	}

	void print20()
	{
		NODE* p = head->next;
		for (int i = 0; i < 20; ++i) {
			if (p == nullptr) break;
			cout << p->v << ", ";
			p = p->next;
		}
		cout << endl;
	}

	void clear()
	{
		NODE* p = head->next;
		while (p != nullptr) {
			NODE* t = p;
			p = p->next;
			delete t;
		}
		tail = head;
		head->next = nullptr;
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
			int value = next->v;

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
	void print20() {
		NODE* p = head->next;
		int count = 0;
		for (int i = 0; i < 20; ++i)
		{
			if (p == nullptr) break;
			cout << p->v << " ";
			p = p->next;
		}
		cout << endl;
	}
};

class alignas(8) SPTR {
public:
	NODE* volatile ptr;
	int	stamp;
	SPTR() : ptr(nullptr), stamp(0) {}
	SPTR(NODE* p, int v) : ptr(p), stamp(v) {}

	NODE* get_ptr()
	{
		return ptr;
	}
	NODE* get_next()
	{
		return ptr->next;
	}
	int get_stamp()
	{
		return stamp;
	}
	const bool operator == (const SPTR rhs)
	{
		return (rhs.ptr == ptr) && (rhs.stamp == stamp);
	}

	const bool operator != (const SPTR rhs)
	{
		return (rhs.ptr != ptr) || (rhs.stamp != stamp);
	}
};

class SLFQUEUE {
	SPTR  head, tail;
	bool CAS(SPTR *ptr, NODE* old_p, NODE* new_p, int old_stamp)
	{
		SPTR old_sptr{ old_p, old_stamp };
		SPTR new_sptr{ new_p, old_stamp + 1 };

		return atomic_compare_exchange_strong(
			reinterpret_cast<atomic_llong*>(ptr),
			reinterpret_cast<long long*>(&old_sptr),
			*reinterpret_cast<long long*>(&new_p));
	}
	bool CAS(NODE* volatile* targetNode, NODE* oldNode, NODE* newNode)
	{
		return atomic_compare_exchange_strong(
			reinterpret_cast<volatile atomic_llong*>(targetNode),
			reinterpret_cast<long long*>(&oldNode),
			reinterpret_cast<long long>(newNode)
		);
	}
public:
	SLFQUEUE()
	{
		head = tail = SPTR{ new NODE{}, 0 };
	}
	void ENQ(int x)
	{
		NODE* e = new NODE(x);
		while (1) {
			SPTR last = tail;
			NODE* next = last.ptr->next;
			if (last != tail) continue;
			if (next == nullptr) {
				if (CAS(&last.ptr->next, nullptr, e)) {
					CAS(&tail, last.ptr, e, last.stamp);
					return;
				}
			}
			else CAS(&tail, last.ptr, next, last.stamp);
		}
	}

	int DEQ()
	{
		while (true) {
			SPTR first = head;
			SPTR last = tail;
			NODE* next = first.ptr->next;
			if (first != head) continue;
			if (nullptr == next) return -1;
			int value = next->v;
			if (first == last) {
				CAS(&tail, last.ptr, next, tail.stamp);
				continue;
			}
			
			if (false == CAS(&head, first.ptr, next, first.stamp))
				continue;
			delete first.ptr;
			return value;
		}

	}

	void print20()
	{
		NODE* p = head.get_next();
		for (int i = 0; i < 20; ++i) {
			if (p == nullptr) break;
			cout << p->v << ", ";
			p = p->next;
		}
		cout << endl;
	}

	void clear()
	{
		NODE* p = head.get_next();
		while (p != nullptr) {
			NODE* t = p;
			p = p->next;
			delete t;
		}
		head = SPTR{};
		tail = head;
	}
};

thread_local int tl_id;

int Thread_id()
{
	return tl_id;
}

typedef SLFQUEUE MY_QUEUE;

constexpr int LOOP = 10000000;

void worker(MY_QUEUE *my_queue, int threadNum, int th_id)
{
	tl_id = th_id;

	for (int i = 0; i < LOOP / threadNum; i++) {
		if ((rand() % 2) || i < 128 / threadNum) {
			my_queue->ENQ(i);
		}
		else {
			my_queue->DEQ();
		}
	}
}

int main()
{
	for (int num_threads = 1; num_threads <= MAX_THREADS; num_threads *= 2) {
		vector <thread> threads;
		MY_QUEUE my_queue;
		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < num_threads; ++i)
			threads.emplace_back(worker, &my_queue, num_threads, i);
		for (auto& th : threads)
			th.join();
		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		auto exec_ms = duration_cast<milliseconds>(exec_t).count();
		my_queue.print20();
		cout << num_threads << " Threads.  Exec Time : " << exec_ms << endl;
	}
}