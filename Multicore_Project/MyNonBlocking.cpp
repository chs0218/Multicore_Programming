#include <iostream>
#include <array>
#include <set>
#include <chrono>
#include <thread>
#include <mutex>
#include <limits>

using namespace std;
using namespace std::chrono;

/*
	���� ��ü
	- Non-blocking�˰����� ����� ���� �ʿ��� ��ü
	���� ��
	- Non-blocking �˰����� ����� �ɷ�
	���ɼ�
	- ��� �˰����� ��Ƽ ������ ������

	�츮�� ����ϴ� ��ǻ�� �޸𸮴� atomic�� �ƴϴ�.
	������ atomic<T>�� ����� atomic�ϰ� ����� �� �ְ� �������� �� �ִ�.
	Ȥ�� ������ ��ġ�� atomic_thread_fence�� �߰��Ѵ�.

	atomic<int> a;
	- a�� ���� ��� ������ atomic���� �����ϸ�, wait-free�� ����ȴ�.
	atomic<vector> a;
	- ������ ����, ������ �ڷᱸ���� atomic�ϰ� ������ �� ����.
	atomic<point> pos;
	- pos�� ���� load store�� atomic
	- ���������� mutex�� ����ؼ� �����Ǳ� ������ lock-free�� �ƴϴ�.

	���� atomic�� ������ �ڷᱸ���� �ʿ��ϸ�?
	- vector, tree, hash-table, priority-queue
	������ ����ȭ���� ����ؼ� Non-blocking���� ��ȯ�ؼ� ����ؾ��Ѵ�.

	����ȭ��?
	
	�ڷᱸ���� ������ Aomic�ϰ� �����ϴ� ��
	- ����/������/��õ����/������/����� ����ȭ�� �����غ��Ҵ�.

	����ȭ�� �����ϱ� ���ؼ��� �⺻ ����ȭ ������� ����ؾ��Ѵ�.
	- ��) �޸� : atomic_load(), atomic_store()
	- ��) LF_SET : ADD(), REMOVE)(, CONTAINS()

	�� �⺻ ����ȭ ������� wait-free Ȥ�� lock-free�̾���Ѵ�.
	- �ƴϸ� wait-free Ȥ�� lock-free�� ������ �� ����.

	atomic_load(), atomic_store()�� Non-Blocking �ڷᱸ���� ����� �񱳰� ��ƴ�.
	- ���� ��ü�� ������ ���غ���.

	���� ��ü��?

	���ο� ����ȭ ������ �����ϴ� ������ ��ü
	
	����ȭ ���� : decide
	- n���� �����尡 decide�� ȣ���� ��, ���� ���� �����Ѵ�.
	- ������ ������� �޼ҵ带 �ѹ� ���Ϸθ� ȣ���Ѵ�.
	- ��� ȣ�⿡ ���� ���� ���� ��ȯ�Ѵ�.
	- ���޵� value �� �ϳ��� ���� ��ȯ�Ѵ�.
	- atomic�ϰ� wait-free�� �����Ѵ�.

	��� �����尡 wait-free�� ���� ����� ��´�.
	���� ���� ������� �� �ϳ��� �����ϰ� ���� ���õǾ����� �˰��Ѵ�.
	- ���� Ȯ���� ���� ó�� ȣ���� �����尡 ����

	���� ����?

	����ȭ ������ �����ϴ� Ŭ���� C�� ���� ��, Ŭ���� C�� atomic �޸𸮸� ������ �����
	n���� �����忡 ���� ���� ��ü�� ������ �� �ִ�. 
	- Ŭ���� C�� n-������ ���� ������ �ذ��Ѵٰ� �Ѵ�.

	Ŭ���� C�� ���� ��
	- C�� �̿��� �ذ� ������ n-������ ���� ���� �� �ִ��� n�� ���Ѵ�.
	- ���� �ִ� n�� �������� �ʴ´ٸ� �� Ŭ������ ���� ���� �����ϴٰ� �Ѵ�.

	�� Ŭ���� C�� �����ؾ��ϴ���?
	- atomic �޸𸮷� n�� �������� ���� ������ �ذ��� �� �ִ°�?
		- atomic_load(), atomic_stor()�� ���� ��ü�� ������ �� �ִ��� ���캸��
		- �ϴ� 2�� �����忡�� ���캸��
		- /.../
		- �ذ��� �� ����.
		- ���� ���� 1�̴�.
	- Queue ��ü�� ������ �ذ� �����Ѱ�?
		- 2�� �����忡�� ���ÿ� Deque���� ��, atomic�ϰ� wait-free�ϰ� �����ϴ� ť�� �ִٰ� ����
		- �����ϴ�! ���� ���� 2���̴�.
		- ������ 3�� �̻� �����忡�� waitr-free�ϰ� �۾��� �� ����.
		- 3�� �̻� �����忡�� �۾��Ϸ���? ���� ���� ��ü�� ����ؾ��Ѵ�.

	���� ���� ��ü
	- �迭�� �����Ǹ� ������ ���Ҹ� atomic�ϰ� ������ �� �ִ� ��ü
	- ���� ��ü���� HW������ �����ϸ� ���ǹ����� ������ �ذ��� �� �ִ�.
		- �׷��� HW���� ����� �ʹ� ũ��.
		- �ذ� �����? RMW ����

	RMW ����
	- �ϵ��� �����ϴ� ����ȭ ������ �� ����
	- Ư�� ��ɾ �ݵ�� �ʿ�

	�޼ҵ� M�� �Լ� f�� ���� RMW�̴�. �� ��
	- �޼ҵ� M�� ���������� ������ �޸��� ���� v���� f(v)�� �ٲٰ� ���� �� v�� ��ȯ�Ѵ�.
	- RMW ������ ����
		- GetAndSet
		- GetAndIncrement
		- GetAndAdd
		- CompareAndSet
		- Get
	- �׵��Լ��� �ƴ� �Լ��� ������ �� �� RMW�� ������� ����(nontrivial)�̶�� �Ѵ�.

	Atomic �޸� read/write �����δ� �Ϲ����� wait-free �ڷᱸ���� ������ �� ����.
		
	CAS�� ���Ѵ� ���� ���� ���´�.
	- ������ ���� ���� ���� �ڷᱸ���� ������ �� �ִ� ����� ������.
	- �׷��ٸ� ���� �� ���Ѵ��� ����ȭ �������� ��� �ڷᱸ���� ������ �� �ִ°�?
		- �����ϴ�!
	LL-SC(Load Linked Store Coniditional)
	- ARM CPU�� CAS �����̴�.

	���� ��ü
	- ��� ��ü�� ����� ���İ�ü�� ��ȯ���� �ִ� ��ü
	- n���� �����忡�� �����ϴ� ���ɰ�ü�� ���� �� n�̻��� ��ü�� ������ ���� �����ϴ�.
		- ���Ѵ��� ���� �� ��ü CAS�� ����ϸ� �����尳���� ������� ���� ��ü�� ������ �� �ִ�.

	������ ����
	- Ŭ���� C ��ü��� ������ �޸𸮷� ��� ��ü�� ����� �������� ��ȯ�ϴ� ���� 
		�����ϴٸ� Ŭ���� C�� �����̴�.

	���� ��ü A�� wait-free�� �����ϰ� �ʹ�!
	- ������ ���� ����: A�� �������̴�.(deterministic)
		- ��� ��ü�� �ʱ���´� �׻� ���� �����̴�.
			- queue<int> a, b, c, d; �ϸ� a, b, c, d�� empty��
		- ���� ���¿��� ���� �Է��� �ָ� �׻� ���� ����� ���� �Ϸ� ���°� ���´�.
			- �ʱ� ���¿��� ���� �Է� ���� ������ ������ �Է��ϸ� �׻� ���� ����� ���´�.
			- �Է� ���� ����Ʈ�� �α�(log)��� �Ѵ�.
	- ������ ���� �غ�
		- ���� ��ü A�� �ִ�.
			- ����ȭ �ϰ��� �ϴ� ��ü�� ���� ��ü�� ����
			- ȣ�� �޼ҵ带 apply �ϳ��� ����
			
			class SeqObject{
			public:
				Response apply(Invocation invoc);
			}
			- Invocation ��ü�� �ִ�.
				- ȣ���ϰ��� �ϴ� ���� ��ü�� �޼ҵ�� �� �Է°��� ���� ��ü
			- Response
				- ���� �޼ҵ� ���� ��� ���� Ÿ���� ������ ��ü
		- Log�� �ִ�.
			- Log�� � ����� � �޼ҵ带 ȣ���ߴ��� ����� ����Ʈ�̴�.
			- ��帶�� ���� ��ü�� �ִ�.



*/

constexpr int MAX_THREADS = 32;
constexpr int NUM_TEST = 4000000;
constexpr int KEY_RANGE = 1000;

enum class METHOD_TYPE
{
	OP_ADD,
	OP_REMOVE,
	OP_CONTAINS,
	OP_CLEAR,
	OP_EMPTY,
	OP_GET20
};

struct Response {
	bool m_bResult;
	vector<int> m_vGet20;
};

struct Invocation {
	METHOD_TYPE m_mtOp;
	int m_nValue;
};

class SeqObject_set {
	set<int> m_stSet;
public:
	const set<int>& GetSet() { return m_stSet; }
	Response Apply(Invocation& inv)
	{
		Response result;
		switch (inv.m_mtOp)
		{
		case METHOD_TYPE::OP_ADD: 
			result.m_bResult = (m_stSet.count(inv.m_nValue) == 0); 
			m_stSet.insert(inv.m_nValue);
			break;
		case METHOD_TYPE::OP_REMOVE:
			result.m_bResult = m_stSet.erase(inv.m_nValue);
			break;
		case METHOD_TYPE::OP_CONTAINS:
			result.m_bResult = m_stSet.contains(inv.m_nValue);
			break;
		case METHOD_TYPE::OP_CLEAR:
			m_stSet.clear();
			break;
		case METHOD_TYPE::OP_EMPTY:
			result.m_bResult = m_stSet.empty();
			break;
		case METHOD_TYPE::OP_GET20:
		{
			int count = 20;
			for (auto& v : m_stSet)
			{
				result.m_vGet20.push_back(v);

				if (--count == 0)
					break;
			}
		}
			break;
		default:
			cout << "Unknown Func" << endl;
			exit(-1);
		}
		return result;
	}
};

struct UNODE;
class CONSENSUS {
	UNODE* ptr;
public:
	CONSENSUS() : ptr(nullptr) {}
	void CAS(UNODE* old_p, UNODE* new_p)
	{
		atomic_compare_exchange_strong(
			reinterpret_cast<atomic_llong*>(&ptr), 
			reinterpret_cast<long long*>(&old_p), 
			reinterpret_cast<long long>(new_p));
	}
	UNODE* decide(UNODE* v)
	{
		CAS(nullptr, v);
		return ptr;
	}
};

thread_local int tl_id;
int THREAD_ID()
{
	return tl_id;
}

struct UNODE
{
	Invocation m_Inv;
	UNODE* next;
	CONSENSUS decide_next;
	volatile int m_nSeq;

	UNODE() { m_nSeq = 0; next = nullptr; }
	~UNODE() { }
	UNODE(Invocation& inv) : m_Inv(inv), next(nullptr) {};
};

class STD_SET {
	SeqObject_set mySet;
public:
	STD_SET()
	{
	}
	~STD_SET() { Init(); }
	void Init()
	{
		Invocation inv{ METHOD_TYPE::OP_CLEAR, 0 };
		Response a = mySet.Apply(inv);
	}
	bool Add(int key)
	{
		Invocation inv{ METHOD_TYPE::OP_ADD, key };
		Response a = mySet.Apply(inv);
		return a.m_bResult;
	}
	bool Remove(int key)
	{
		Invocation inv{ METHOD_TYPE::OP_REMOVE, key };
		Response a = mySet.Apply(inv);
		return a.m_bResult;
	}
	bool Contains(int key)
	{
		Invocation inv{ METHOD_TYPE::OP_CONTAINS, key };
		Response a = mySet.Apply(inv);
		return a.m_bResult;
	}
	void PrintTwenty() {
		int count = 20;
		for (auto& it : mySet.GetSet())
		{
			cout << it << ", ";
			--count;

			if (count == 0)
				break;
		}
		cout << endl;
	}
};

class LF_UNIV_STD_SET{
private:
	UNODE* head[MAX_THREADS];
	UNODE tail;
public:
	LF_UNIV_STD_SET() {
		tail.m_nSeq = 0;
		for (int i = 0; i < MAX_THREADS; ++i) head[i] = &tail;
	}
	UNODE* max_node()
	{
		UNODE* max_n = head[0];
		for (int i = 1; i < MAX_THREADS; ++i)
			if (max_n->m_nSeq < head[i]->m_nSeq)
				max_n = head[i];
		return max_n;
	}
	Response Apply(Invocation invoc, int thread_id) {
		int i = thread_id;
		UNODE prefer = UNODE(invoc);
		while (prefer.m_nSeq == 0) {
			UNODE* before = max_node();
			UNODE* after = before->decide_next.decide(&prefer);
			before->next = after; after->m_nSeq = before->m_nSeq + 1;
			head[i] = after;
		}
		SeqObject_set myObject;
		UNODE* current = tail.next;
		while (current != &prefer) {
			myObject.Apply(current->m_Inv);
			current = current->next;
		}
		return myObject.Apply(current->m_Inv);
	}
};

class LF_STD_SET {
	LF_UNIV_STD_SET mySet;
public:
	LF_STD_SET()
	{
	}
	~LF_STD_SET() { Init(); }
	void Init()
	{
		Invocation inv{ METHOD_TYPE::OP_CLEAR, 0 };
		Response a = mySet.Apply(inv, THREAD_ID());
	}
	bool Add(int key)
	{
		Invocation inv{ METHOD_TYPE::OP_ADD, key };
		Response a = mySet.Apply(inv, THREAD_ID());
		return a.m_bResult;
	}
	bool Remove(int key)
	{
		Invocation inv{ METHOD_TYPE::OP_REMOVE, key };
		Response a = mySet.Apply(inv, THREAD_ID());
		return a.m_bResult;
	}
	bool Contains(int key)
	{
		Invocation inv{ METHOD_TYPE::OP_CONTAINS, key };
		Response a = mySet.Apply(inv, THREAD_ID());
		return a.m_bResult;
	}
	void PrintTwenty() {
		Invocation inv{ METHOD_TYPE::OP_GET20, 0 };
		Response a = mySet.Apply(inv, THREAD_ID());
		int count = 20;
		for (auto& v : a.m_vGet20)
		{
			cout << v << ", ";
		}
		cout << endl;
	}
};

#define MY_SET LF_STD_SET

class HISTORY {
public:
	int op;
	int i_value;
	bool o_value;
	HISTORY(int o, int i, bool re) : op(o), i_value(i), o_value(re) {}
};

void ThreadFunc(MY_SET* mySet, vector<HISTORY>* history, int num_thread, int thread_id)
{
	tl_id = thread_id;

	int key;
	for (int i = 0; i < NUM_TEST / num_thread; ++i)
	{
		switch (rand() % 3) {
		case 0: key = rand() % KEY_RANGE;
			mySet->Add(key);
			break;
		case 1: key = rand() % KEY_RANGE;
			mySet->Remove(key);
			break;
		case 2: key = rand() % KEY_RANGE;
			mySet->Contains(key);
			break;
		default: cout << "Error\n";
			exit(-1);
		}
	}
}

void ThreadFunc_Check(MY_SET* mySet, vector<HISTORY>* history, int num_threads, int thread_id)
{
	tl_id = thread_id;
	for (int i = 0; i < NUM_TEST / num_threads; ++i) {
		int op = rand() % 3;
		switch (op) {
		case 0: {
			int v = rand() % KEY_RANGE;
			history->emplace_back(0, v, mySet->Add(v));
			break;
		}
		case 1: {
			int v = rand() % KEY_RANGE;
			history->emplace_back(1, v, mySet->Remove(v));
			break;
		}
		case 2: {
			int v = rand() % KEY_RANGE;
			history->emplace_back(2, v, mySet->Contains(v));
			break;
		}
		}
	}
}

void Check_History(MY_SET* mySet, array <vector <HISTORY>, MAX_THREADS>& history, int num_threads)
{
	array <int, KEY_RANGE> survive = {};
	cout << "Checking Consistency : ";
	if (history[0].size() == 0) {
		cout << "No history.\n";
		return;
	}
	for (int i = 0; i < num_threads; ++i) {
		for (auto& op : history[i]) {
			if (false == op.o_value) continue;
			if (op.op == 3) continue;
			if (op.op == 0) survive[op.i_value]++;
			if (op.op == 1) survive[op.i_value]--;
		}
	}
	for (int i = 0; i < KEY_RANGE; ++i) {
		int val = survive[i];
		if (val < 0) {
			cout << "ERROR. The value " << i << " removed while it is not in the set.\n";
			exit(-1);
		}
		else if (val > 1) {
			cout << "ERROR. The value " << i << " is added while the set already have it.\n";
			exit(-1);
		}
		else if (val == 0) {
			if (mySet->Contains(i)) {
				cout << "ERROR. The value " << i << " should not exists.\n";
				exit(-1);
			}
		}
		else if (val == 1) {
			if (false == mySet->Contains(i)) {
				cout << "ERROR. The value " << i << " shoud exists.\n";
				exit(-1);
			}
		}
	}
	cout << " OK\n";
}

int main()
{
	for (int i = 1; i <= MAX_THREADS; i *= 2)
	{
		LF_STD_SET mySet;

		vector<thread> threads;
		array<vector <HISTORY>, MAX_THREADS> history;
		auto start_t = high_resolution_clock::now();

		for (int j = 0; j < i; ++j)
			threads.emplace_back(ThreadFunc_Check, &mySet, &history[j], i, j);

		for (thread& t : threads)
			t.join();

		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;

		mySet.PrintTwenty();
		printf("THREAD_NUM = %d\n", i);
		printf("EXEC TIME = %lld msec\n", duration_cast<milliseconds>(exec_t).count());
		Check_History(&mySet, history, i);
		printf("\n");
	}

	cout << "======== SPEED CHECK =============\n";

	for (int i = 1; i <= MAX_THREADS; i *= 2)
	{
		LF_STD_SET mySet;

		vector<thread> threads;
		array<vector <HISTORY>, MAX_THREADS> history;
		auto start_t = high_resolution_clock::now();

		for (int j = 0; j < i; ++j)
			threads.emplace_back(ThreadFunc, &mySet, &history[j], i, j);

		for (thread& t : threads)
			t.join();

		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;

		mySet.PrintTwenty();
		printf("THREAD_NUM = %d\n", i);
		printf("EXEC TIME = %lld msec\n", duration_cast<milliseconds>(exec_t).count());
		Check_History(&mySet, history, i);
		printf("\n");
	}
	return 0;
}