/*
��尡 ������ �浹 Ȯ���� ������, stack�� queue�� ����(���� ��) �浹�ϰ� �ȴ�.
�̷����� ���ļ��� ����, ��Ƽ �����忡�� ������ �� ����!!
=> ���� ��������

ABA ������ ���� Ȯ���� �� ����.
������ �׷��� �ؼ� LFStack�� ���� ���� ������ ����.

������ ���Ұ�� CAS ���� �� ��� ��õ��ϴ� ���� ��ü �ý��ۿ� �ǿ����� �ش�.

BACK OFF ������ ���

Back Off��?
������ ���� ��� ������ ������!
CAS�� �������� ��� ������ �Ⱓ ���� ������ ���߾��ٰ� �簳

������ �Ⱓ�̶�?
ó������ ª��, ��� �����ϸ� ���� ���
Thread���� �Ⱓ�� �ٸ��� �ؾ��Ѵ�.

��, �ü���� ȣ���ϸ� �������� ���� ������ ������ �� �ִ�
�׷��ٸ� ���� CPU Counter�� �̿��غ���

RDTSC => Read Time Stack Counter(���� eax������) 
mov start, eax (start�� ����)

������ RDTSC�� CPU ������ �ƴ� CPU Ŭ�� �ӵ��� ����Ѵ�.
���� �������� �޸𸮸� �ƿ� �������� �ʴ� ���� �ٶ����Ѵ�.

������� �ۼ��� ��ũ�� �ؾ��Ѵ�? << ���߿� �˾ƺ���.

������ Lock Free �˰����� ������尡 ���� �ھ ���� �ʱ� ������ BackOff�� ������ �� ������ �ʴ´�.
������ �ھ� ������ ���ų�, BackOff �浹 ������尡 Ŭ��� BackOff�� ���� ���� ����� ���� �� �ִ�.

�Ұ�?
Queue�� Stack�� ����Ʈ�� ���� �κп��� ���� �浹�� �����!!
������ ����ȭ Ÿ���� ����ȭ�� �Ұ����ϴ�..(�����尡 ���������� ���� ����� X)
Queue�� �浹�� �л������ Stack��? ��

��¿�� ���°�? �׷��� �ʴ�!!
���� �˰����� Stack�� �ھ ���������� ������ ���ȴ�.
�׷� � ����� ����ؾ��ϴ°�

�Ұ�!! (�浹�� �Ұ��Ѵ�)
���̵��: Stack���� �浹�� �ʼ��ΰ�?
Push�� Pop�� ���ÿ� �߻��ϴ� ���, Stack�� �Ȱǵ��̰� �� �� ���� �ʴ°�?
Stack�� ���� �ʰ� ���� Data�� �ְ� �޵��� �Ѵ�???
�̶�, Lock Free�� �ְ� �޾ƾ��Ѵ�.
Stack�� ������� �ʾƾ��ϴ� ������ �ٸ� ��ü�� �ʿ��ϴ�.
���� ������� ����� ������ ��ü�� �������� �غ��Ѵ�.

������ ��ȯ��ü
��ȯ�� �õ��ϰ� �����Ұ�� Stack�� ����
��ȯ�ڰ� ������ �������� ���� ������ ���� ���ɼ��� ũ��.

��ȯ�ڿ��� ABA ������ �Ͼ����, ��ȯ����� �ٲ� �� ���̰� ���� �״���̱� ������ �ƹ��� ������ ����.
�ʹ� ª�� ��ȯ �ð��� �Ҵ��ϸ� �׻� �����ϰ� �ǹǷ� �ð����� �Ⱓ�� �� ���ؾ��Ѵ�.

���� ���¿� ���� ��ȯ�� ������ �޶����� �غ���.
=> �Ұ� �迭





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

#define ST_EMPTY	0x00000000
#define ST_WAITING	0x40000000
#define ST_BUSY		0x80000000
#define VALUE_MASK	0x3FFFFFFF
#define STATUS_MASK 0xC0000000
constexpr int LOOP_LIMIT = 1000;

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
		if (limit != 0) delay = rand() % limit;
		if (limit == 0) limit = 1;
		else limit *= 2;
		if (limit > maxDelay) limit = maxDelay;
		this_thread::sleep_for(chrono::microseconds(delay));;
	}
	void reduce() {
		limit /= 2;
	}
};
class EXCHANGER {
	volatile unsigned int slot;
public:
	EXCHANGER() : slot(ST_EMPTY) {}
	bool CAS(unsigned old_slot, unsigned new_status, unsigned new_value){
		return atomic_compare_exchange_strong(
			reinterpret_cast<volatile atomic_uint*>(&slot), &old_slot, new_status | new_value
		);
	}
	int exchange(int v) {
		if (v == -1) {
			v = v & VALUE_MASK;
		}

		if (STATUS_MASK & v != ST_EMPTY) {
			cout << "Value is too big!!\n";
			return;
		}

		int loop_counter = 0;
		while (true) {
			if (LOOP_LIMIT < loop_counter++)
				return -3;	// ��ȯ ����, �浹�� �ʹ� ���ϴ�

			unsigned curr = slot;
			unsigned status = curr & STATUS_MASK;
			unsigned value = curr & VALUE_MASK;
			switch (status) {
			case ST_EMPTY:
				if (CAS(curr, ST_WAITING, v)) {
					int count = 0;
					while (STATUS_MASK & slot == ST_WAITING) {
						if (LOOP_LIMIT < count++) {
							if(CAS(ST_WAITING | v, status, value))
								return -2;	// ��ȯ ����, ��ȯ ��밡 ����.
							else {
								break;
							}
						}
					}
					unsigned ret = VALUE_MASK & slot;
					slot = ST_EMPTY;
					if (ret == VALUE_MASK)
						return -1;
					return ret;
				}
				else {
					continue;
				}
				break;
			case ST_WAITING:
				if (CAS(curr, ST_BUSY, v)) {
					if (value == VALUE_MASK)
						value = -1;
					return value;
				}
				else {
					continue;
				}
				break;
			case ST_BUSY:
				continue;
			default:
				cout << "Unknown Status Error\n";
				exit(-1);
			}
		}
	}
};
class EliminationArray {
	volatile int range;
	EXCHANGER exchanger[MAX_THREADS];
public:
	EliminationArray() { range = 1; }
	~EliminationArray() {}
	bool CAS(int old_v, int new_v) {
		return atomic_compare_exchange_strong(
			reinterpret_cast<volatile atomic_int*>(&range),
			&old_v,
			new_v
		);
	}
	int Visit(int value) {
		int old_range = range;
		int slot = rand() % range;
		int ret = exchanger[slot].exchange(value);

		if ((ret == -2) && (old_range > 1)) // WAIT TIME OUT
			CAS(old_range, old_range - 1);
		if ((ret == -3) && (old_range < MAX_THREADS / 2))
			CAS(old_range, old_range + 1); // WAIT TOO BUSY
		return ret;
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

typedef LFBOSTACK MY_STACK;

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