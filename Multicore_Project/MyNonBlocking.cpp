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
	합의 객체
	- Non-blocking알고리즘을 만들기 위해 필요한 객체
	합의 수
	- Non-blocking 알고리즘을 만드는 능력
	만능성
	- 모든 알고리즘을 멀티 쓰레드 무대기로

	우리가 사용하는 컴퓨터 메모리는 atomic이 아니다.
	하지만 atomic<T>를 사용해 atomic하게 사용할 수 있게 컴파일할 수 있다.
	혹은 적절한 위치에 atomic_thread_fence를 추가한다.

	atomic<int> a;
	- a에 대한 모든 연산을 atomic으로 수행하며, wait-free로 수행된다.
	atomic<vector> a;
	- 컴파일 에러, 복잡한 자료구조는 atomic하게 변경할 수 없다.
	atomic<point> pos;
	- pos에 대한 load store가 atomic
	- 내부적으로 mutex를 사용해서 구현되기 때문에 lock-free가 아니다.

	만약 atomic한 복잡한 자료구조가 필요하면?
	- vector, tree, hash-table, priority-queue
	적절한 동기화법을 사용해서 Non-blocking으로 변환해서 사용해야한다.

	동기화란?
	
	자료구조의 동작을 Aomic하게 구현하는 것
	- 성긴/세밀한/낙천적인/게으른/비멈춤 동기화로 구현해보았다.

	동기화를 구현하기 위해서는 기본 동기화 연산들을 사용해야한다.
	- 예) 메모리 : atomic_load(), atomic_store()
	- 예) LF_SET : ADD(), REMOVE)(, CONTAINS()

	이 기본 동기화 연산들은 wait-free 혹은 lock-free이어야한다.
	- 아니면 wait-free 혹은 lock-free로 구현할 수 없다.

	atomic_load(), atomic_store()와 Non-Blocking 자료구조의 관계는 비교가 어렵다.
	- 합의 객체를 정의해 비교해본다.

	합의 객체란?

	새로운 동기화 연산을 제공하는 가상의 객체
	
	동기화 연산 : decide
	- n개의 쓰레드가 decide를 호출할 때, 문제 없이 동작한다.
	- 각각의 스레드는 메소드를 한번 이하로만 호출한다.
	- 모든 호출에 대해 같은 값을 반환한다.
	- 전달된 value 중 하나의 값을 반환한다.
	- atomic하고 wait-free로 동작한다.

	모든 쓰레드가 wait-free로 같은 결론을 얻는다.
	여러 경쟁 쓰레드들 중 하나를 선택하고 누가 선택되었는지 알게한다.
	- 높은 확률로 제일 처음 호출한 쓰레드가 선택

	합의 수란?

	동기화 연산을 제공하는 클래스 C가 있을 때, 클래스 C와 atomic 메모리를 여러개 사용해
	n개의 스레드에 대한 합의 객체를 구현할 수 있다. 
	- 클래스 C가 n-스레드 합의 문제를 해결한다고 한다.

	클래스 C의 합의 수
	- C를 이용해 해결 가능한 n-스레드 합의 문제 중 최대의 n을 말한다.
	- 만약 최대 n이 존재하지 않는다면 그 클래스의 합의 수를 무한하다고 한다.

	왜 클래스 C를 구현해야하느냐?
	- atomic 메모리로 n개 스레드의 합의 문제를 해결할 수 있는가?
		- atomic_load(), atomic_stor()로 합의 객체를 구현할 수 있는지 살펴보자
		- 일단 2개 쓰레드에서 살펴보자
		- /.../
		- 해결할 수 없다.
		- 합의 수는 1이다.
	- Queue 객체가 있으면 해결 가능한가?
		- 2개 스레드에서 동시에 Deque했을 때, atomic하고 wait-free하게 동작하는 큐가 있다고 가정
		- 가능하다! 합의 수는 2개이다.
		- 하지만 3개 이상 쓰레드에서 waitr-free하게 작업할 수 없다.
		- 3개 이상 쓰레드에서 작업하려면? 다중 대입 객체를 사용해야한다.

	다중 대입 객체
	- 배열로 구성되며 복수의 원소를 atomic하게 변경할 수 있는 객체
	- 대입 객체들을 HW적으로 지원하면 합의문제를 무대기로 해결할 수 있다.
		- 그러나 HW구현 비용이 너무 크다.
		- 해결 방법은? RMW 연산

	RMW 연산
	- 하드웨어가 지원하는 동기화 연산의 한 종류
	- 특수 명령어가 반드시 필요

	메소드 M은 함수 f에 대한 RMW이다. 의 뜻
	- 메소드 M이 원자적으로 현재의 메모리의 값을 v에서 f(v)로 바꾸고 원래 값 v를 반환한다.
	- RMW 연산의 종류
		- GetAndSet
		- GetAndIncrement
		- GetAndAdd
		- CompareAndSet
		- Get
	- 항등함수가 아닌 함수를 지원할 때 그 RMW를 명백하지 않은(nontrivial)이라고 한다.

	Atomic 메모리 read/write 만으로는 일반적인 wait-free 자료구조를 구현할 수 없다.
		
	CAS가 무한대 합의 수를 갖는다.
	- 임의의 합의 수를 갖는 자료구조를 구현할 수 있는 희망을 가진다.
	- 그렇다면 합의 수 무한대인 동기화 연산으로 모든 자료구조를 구현할 수 있는가?
		- 가능하다!
	LL-SC(Load Linked Store Coniditional)
	- ARM CPU의 CAS 연산이다.

	만능 객체
	- 어떠한 객체든 무대기 병렬객체로 변환시켜 주는 객체
	- n개의 스레드에서 동작하는 만능객체는 합의 수 n이상의 객체만 있으면 구현 가능하다.
		- 무한대의 합의 수 객체 CAS를 사용하면 쓰레드개수에 상관없이 만능 객체를 구현할 수 있다.

	만능의 정의
	- 클래스 C 객체들과 원자적 메모리로 모든 객체를 무대기 구현으로 변환하는 것이 
		가능하다면 클래스 C는 만능이다.

	순차 객체 A를 wait-free로 구현하고 싶다!
	- 변형을 위한 조건: A는 결정적이다.(deterministic)
		- 모든 객체의 초기상태는 항상 같은 상태이다.
			- queue<int> a, b, c, d; 하면 a, b, c, d는 empty다
		- 같은 상태에서 같은 입력을 주면 항상 같은 결과와 같은 완료 상태가 나온다.
			- 초기 상태에서 같은 입력 값을 동일한 순서로 입력하면 항상 같은 결과가 나온다.
			- 입력 값의 리스트를 로그(log)라고 한다.
	- 변형을 위한 준비
		- 순차 객체 A가 있다.
			- 병렬화 하고자 하는 객체를 감싼 객체를 생성
			- 호출 메소드를 apply 하나로 통일
			
			class SeqObject{
			public:
				Response apply(Invocation invoc);
			}
			- Invocation 객체가 있다.
				- 호출하고자 하는 원래 객체의 메소드와 그 입력값을 갖는 객체
			- Response
				- 여러 메소드 들의 결과 값의 타입을 압축한 객체
		- Log가 있다.
			- Log는 어떤 값들로 어떤 메소드를 호출했는지 기록한 리스트이다.
			- 노드마다 합의 객체가 있다.



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