/*
노드가 많으면 충돌 확률이 적으나, stack과 queue는 많이(거의 다) 충돌하게 된다.
이로인해 병렬성이 적어, 멀티 쓰레드에서 빨라질 수 없다!!
=> 순차 병목현상

ABA 문제가 생길 확률이 더 높다.
하지만 그렇다 해서 LFStack을 쓰지 않을 이유가 없다.

경쟁이 심할경우 CAS 실패 시 계속 재시도하는 것은 전체 시스템에 악영향을 준다.

BACK OFF 스택을 사용

Back Off란?
경쟁이 심할 경우 경쟁을 줄이자!
CAS가 실패했을 경우 적절한 기간 동안 실행을 멈추었다가 재개

적절한 기간이란?
처음에는 짧게, 계속 실패하면 점점 길게
Thread마다 기간은 다르게 해야한다.

단, 운영체제를 호출하면 오버헤드로 인해 성능이 감소할 수 있다
*/

#include <iostream>
#include <array>
#include <vector>
#include <set>
#include <unordered_set>
#include <chrono>
#include <thread>
#include <mutex>
#include <limits>

using namespace std;
using namespace chrono;

constexpr int MAX_THREADS = 32;
constexpr int NUM_TEST = 1000'0000;

atomic_int stack_size = 0;

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
class BackOff {
	int minDelay, maxDelay;
	int limit;
public:
	BackOff(int min, int max)
		: minDelay(min), maxDelay(max), limit(min) {}
	void relax() {
		int delay = 0;
		if(limit == 0) limit = 1;
		delay = rand() % limit;
		limit *= 2;
		if (limit > maxDelay) limit = maxDelay;
		this_thread::sleep_for(chrono::microseconds(delay));;
	}
	void reduce() {
		limit /= 2;
	}
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

			if (CAS(&top, ptr, ptr->next))
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
class LFBOSTACK {
	BackOff backoff{ 1, MAX_THREADS };
	NODE* volatile top;
public:
	LFBOSTACK()
	{
		top = nullptr;
	}
	~LFBOSTACK() { }
	void PUSH(int x)
	{
		NODE* e = new NODE(x);
		bool first_time = true;

		while (1) {
			e->next = top;

			if (CAS(&top, e->next, e)) {
				if (first_time)backoff.reduce();
				return;
			}

			first_time = false;
			backoff.relax();
		}
	}
	int POP()
	{
		while (1) {
			NODE* ptr = top;
			bool first_time = true;

			if (ptr == nullptr)
				return -2;

			if (CAS(&top, ptr, ptr->next))
			{
				if (first_time)backoff.reduce();
				return ptr->key;
			}

			first_time = false;
			backoff.relax();
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

void worker(MY_STACK* my_stack, int threadNum, int th_id)
{
	tl_id = th_id;

	for (int i = 0; i < NUM_TEST / threadNum; i++) {
		if ((rand() % 2) || i < 1024 / threadNum) {
			my_stack->PUSH(i);
		}
		else {
			my_stack->POP();
		}
	}
}

struct HISTORY {
	vector <int> push_values, pop_values;
};

void worker_check(MY_STACK* my_stack, int num_threads, int th_id, HISTORY& h)
{
	tl_id = th_id;
	for (int i = 0; i < NUM_TEST / num_threads; i++) {
		if ((rand() % 2) || i < 128 / num_threads) {
			h.push_values.push_back(i);
			stack_size++;
			my_stack->PUSH(i);
		}
		else {
			stack_size--;
			int res = my_stack->POP();
			if (res == -2) {
				stack_size++;
				if (stack_size > num_threads) {
					cout << "ERROR Non_Empty Stack Returned NULL\n";
					exit(-1);
				}
			}
			else h.pop_values.push_back(res);
		}
	}
}

void check_history(MY_STACK& my_stack, vector <HISTORY>& h)
{
	unordered_multiset <int> pushed, poped, in_stack;

	for (auto& v : h)
	{
		for (auto num : v.push_values) pushed.insert(num);
		for (auto num : v.pop_values) poped.insert(num);
		while (true) {
			int num = my_stack.POP();
			if (num == -2) break;
			poped.insert(num);
		}
	}
	for (auto num : pushed) {
		if (poped.count(num) < pushed.count(num)) {
			cout << "Pushed Number " << num << " does not exists in the STACK.\n";
			exit(-1);
		}
		if (poped.count(num) > pushed.count(num)) {
			cout << "Pushed Number " << num << " is poped more than " << poped.count(num) - pushed.count(num) << " times.\n";
			exit(-1);
		}
	}
	for (auto num : poped)
		if (pushed.count(num) == 0) {
			std::multiset <int> sorted;
			for (auto num : poped)
				sorted.insert(num);
			cout << "There was elements in the STACK no one pushed : ";
			int count = 20;
			for (auto num : sorted)
				cout << num << ", ";
			cout << endl;
			exit(-1);

		}
	cout << "NO ERROR detectd.\n";
}

int main()
{
	cout << "======== Error Checking =========\n";
	for (int num_threads = 1; num_threads <= MAX_THREADS; num_threads *= 2) {
		vector <thread> threads;
		vector <HISTORY> log(num_threads);
		MY_STACK my_stack;
		stack_size = 0;
		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < num_threads; ++i)
			threads.emplace_back(worker_check, &my_stack, num_threads, i, std::ref(log[i]));
		for (auto& th : threads)
			th.join();
		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		auto exec_ms = duration_cast<milliseconds>(exec_t).count();
		my_stack.PrintTwenty();
		cout << num_threads << " Threads.  Exec Time : " << exec_ms << endl;
		check_history(my_stack, log);
	}

	cout << "======== BENCHMARKING =========\n";
	for (int num_threads = 1; num_threads <= MAX_THREADS; num_threads *= 2) {
		vector <thread> threads;
		MY_STACK my_stack;
		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < num_threads; ++i)
			threads.emplace_back(worker, &my_stack, num_threads, i);
		for (auto& th : threads)
			th.join();
		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		auto exec_ms = duration_cast<milliseconds>(exec_t).count();
		my_stack.PrintTwenty();
		cout << num_threads << " Threads.  Exec Time : " << exec_ms << endl;
	}
}