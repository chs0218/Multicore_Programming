#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include <limits>

using namespace std;
using namespace std::chrono;

/*
Non Blocking 자료 구조를 제작해보자
최종적으로 Lock-Free 자료구조를 제작한다.

Set을 Non Blocking 자료구조로 제작해본다.

field
- key: 리스트에 저장 되는 값
- next: 다음 노드의 포인터

method
- add(x): 집합에 x를 추가하고 성공시 true를 반환
- remove(x): 집합에서 x를 제거하고 성공시 true를 반환
- contains(x): 집합에 x가 있으면 true를 반환

- 검색의 효율성을 위해 MAX_INT, -MAX_INT 값을 가지는 
	보초 노드 Head, Tail을 추가한다.

구현 차례
- 성긴 동기화(Lock 하나로 동기화 객체 전체를 감싸는 경우)
- 세밀한 동기화
- 낙천적인 동기화
- 게으른 동기화
- 비멈춤 동기화

성긴 동기화
- 리스트는 하나의 Lock을 가지고 있으며 모든 method 호출은
	이 Lock을 이용해 Critical Section으로 진행
- 경쟁이 낮은 경우 좋은 선택이지만 경쟁이 높아질 수록 성능이 저하
- Blocking 알고리즘

new와 delete는 내부적으로 locking을 사용하기 때문에 
멀티쓰레드 프로그래밍에서는 잘 사용하지 않는다.

Lock, Unlock을 사용하면 병행성이 제한된다.
병행성을 향상시킬 수 있는 방법을 찾아보자.

세밀한 동기화
- 전체 리스트를 한꺼번에 잠그는 것보다 개별 노드를 잠가 병행성을 향상시킨다.

노드에 Mutex를 넣고 전역 Lock을 뺀다.
Head 부터 Node 이동을 할 때 Lock을 잠그면서 이동한다.

Lock/Unlock 횟수가 너무 많아 성능이 좋지 않다.
하지만 병렬성이 증가를 해 점점 빨라지는 모습을 관찰할 수 있다.
head의 bottlelack 때문에 쓰레드 개수가 일정이상에서 증가해도 빨라지진 않는다.

그렇다면 Lock/Unlock의 횟수를 어떻게 줄일 수 있을까?
- 이동할 때 잠금을 하지 않는다!!
- Add/Remove를 위해 pred를 수정하기 전에 pred를 잠근다.

낙천적 동기화
- 이동시 잠금을 하지 않는다.
- 하지만 단순히 잠금을 하지 않으면 프로그램이 죽는다.
- Crash(또는 무한루프)의 경우 제거된 Node의 next가 다른 값으로 들어가게 되서 문제가 생긴다.
	- 제거된 Node의 next가 crash를 발생시키는 값을 갖지 않게 한다.
	- 제거된 Node라도 next를 따라가면 Tail이 나오게한다.
- Node가 이상한 위치로 가서 붙으면 오동작을 한다.
	- pred와 curr을 잘못 잠갔을 경우 처음부터 다시 실행한다.
	- pred와 curr을 잠갔을 때, 제대로 잠갔는지 검사해 확인한다.
- 제거된 노드를 delete하지 않는다.
	- next 필드는 오염되지 않고 쫓아가면 Tail을 만나게 된다.
	- 단, 메모리가 낭비되는데 나중에 해결한다.
	- 자바 환경에서는 더 이상 사용되지 않는 Node가 자동적으로 삭제되서 문제가 없다.
- Validation 조건
	- 잠겨진 pred와 curr가 제거되지 않았고
	- pred와 curr사이에 다른 노드가 끼어들지 않았다.
- Validate가 실패하면 처음부터 다시 실행하기 때문에 다른 스레드가 계속 수정해 재시도를 할경우
	기아를 겪을 수 있으나 흔치 않은 경우이기 때문에 실제로는 잘 동작할 가능성이 크다.
*/

class my_mutex
{
public:
	void lock(){}
	void unlock(){}
};


class NODE {
public:
	int key;
	NODE* next;
	mutex nlock;
	NODE() { next = NULL; }
	NODE(int key_value) {
		next = NULL;
		key = key_value;
	}
	void lock() { nlock.lock(); }
	void unlock() { nlock.unlock(); }
	~NODE() {}
};

class CSET {
	NODE head, tail;
	mutex glock;
public:
	CSET()
	{
		head.key = numeric_limits<int>::min();
		tail.key = numeric_limits<int>::max();
		head.next = &tail;
	}
	~CSET() { Init(); }
	void Init()
	{
		NODE* ptr;
		while (head.next != &tail) {
			ptr = head.next;
			head.next = head.next->next;
			delete ptr;
		}
	}
	bool Add(int key)
	{
		NODE* prev, * curr;
		prev = &head;
		glock.lock();
		curr = prev->next;
		while (curr->key < key) {
			prev = curr;
			curr = curr->next;
		}
		if (key == curr->key) {
			glock.unlock();
			return false;
		}
		else {
			NODE* node = new NODE(key);	// 이 또한 빼면 좋다
			node->next = curr;
			prev->next = node;
			glock.unlock();
			return true;
		}
	}
	bool Remove(int key)
	{
		NODE* prev, * curr;
		prev = &head;
		glock.lock();
		curr = prev->next;
		while (curr->key < key) {
			prev = curr;
			curr = curr->next;
		}
		if (key == curr->key) {
			prev->next = curr->next;
			glock.unlock();
			delete curr;	// Unlock()을 한 뒤에 delete하는 편이 좋다. delete 명령의 운영체제의 호출때문
			return true;
		}
		else {
			glock.unlock();
			return false;
		}

	}
	bool Contains(int key)
	{
		NODE* prev, * curr;
		prev = &head;
		glock.lock();
		curr = prev->next;
		while (curr->key < key) {
			prev = curr;
			curr = curr->next;
		}
		if (key == curr->key) {
			glock.unlock();
			return true;
		}
		else {
			glock.unlock();
			return false;
		}
	}
	void PrintTwenty() {
		NODE* prev, * curr;
		prev = &head;
		curr = prev->next;
		int count = 0;

		while (count < 20) {
			cout << curr->key << ", ";
			prev = curr;
			curr = curr->next;
			++count;
		}
		cout << endl;
	}
};
class FSET {
	NODE head, tail;
public:
	FSET()
	{
		head.key = numeric_limits<int>::min();
		tail.key = numeric_limits<int>::max();
		head.next = &tail;
	}
	~FSET() { Init(); }
	void Init()
	{
		NODE* ptr;
		while (head.next != &tail) {
			ptr = head.next;
			head.next = head.next->next;
			delete ptr;
		}
	}
	bool Add(int key)
	{
		NODE* prev, * curr;
		prev = &head;
		prev->lock();
		curr = prev->next;
		curr->lock();
		while (curr->key < key) {
			prev->unlock();
			prev = curr;
			curr = curr->next;
			curr->lock();
		}
		if (key == curr->key) {
			prev->unlock();
			curr->unlock();
			return false;
		}
		else {
			NODE* node = new NODE(key);	// 이 또한 빼면 좋다
			node->next = curr;
			prev->next = node;
			prev->unlock();
			curr->unlock();
			return true;
		}
	}
	bool Remove(int key)
	{
		NODE* prev, * curr;
		prev = &head;
		prev->lock();
		curr = prev->next;
		curr->lock();
		while (curr->key < key) {
			prev->unlock();
			prev = curr;
			curr = curr->next;
			curr->lock();
		}
		if (key == curr->key) {
			prev->next = curr->next;
			prev->unlock();
			curr->unlock();
			delete curr;
			return true;
		}
		else {
			prev->unlock();
			curr->unlock();
			return false;
		}

	}
	bool Contains(int key)
	{
		NODE* prev, * curr;
		prev = &head;
		prev->lock();
		curr = prev->next;
		curr->lock();
		while (curr->key < key) {
			prev->unlock();
			prev = curr;
			curr = curr->next;
			curr->lock();
		}
		if (key == curr->key) {
			prev->unlock();
			curr->unlock();
			return true;
		}
		else {
			prev->unlock();
			curr->unlock();
			return false;
		}
	}
	void PrintTwenty() {
		NODE* prev, * curr;
		prev = &head;
		curr = prev->next;
		int count = 0;

		while (count < 20) {
			cout << curr->key << ", ";
			prev = curr;
			curr = curr->next;
			++count;
		}
		cout << endl;
	}
};
class OSET {
	NODE head, tail;
public:
	OSET()
	{
		head.key = numeric_limits<int>::min();
		tail.key = numeric_limits<int>::max();
		head.next = &tail;
	}
	~OSET() { Init(); }
	
	void Init()
	{
		NODE* ptr;
		while (head.next != &tail) {
			ptr = head.next;
			head.next = head.next->next;
			delete ptr;
		}
	}
	bool Validate(NODE* prev, NODE* curr)
	{
		NODE* node = &head;
		while (node->key <= prev->key) {
			if (node == prev)
				return node->next == curr;
			node = node->next;
		}
		return false;
	}
	bool Add(int key)
	{
	tryAgain:
		NODE* prev, * curr;

		prev = &head;
		curr = prev->next;
		while (curr->key < key) {
			prev = curr;
			curr = curr->next;
		}

		prev->lock();
		curr->lock();

		if (!Validate(prev, curr))
		{
			prev->unlock();
			curr->unlock();
			goto tryAgain;
		}

		if (key == curr->key) {
			prev->unlock();
			curr->unlock();
			return false;
		}
		else {
			NODE* node = new NODE(key);	// 이 또한 빼면 좋다
			node->next = curr;
			prev->next = node;
			prev->unlock();
			curr->unlock();
			return true;
		}
	}
	bool Remove(int key)
	{
	tryAgain:
		NODE* prev, * curr;

		prev = &head;
		curr = prev->next;
		while (curr->key < key) {
			prev = curr;
			curr = curr->next;
		}

		prev->lock();
		curr->lock();
		
		if (!Validate(prev, curr))
		{
			prev->unlock();
			curr->unlock();
			goto tryAgain;
		}

		if (key == curr->key) {
			prev->next = curr->next;
			prev->unlock();
			curr->unlock();
			delete curr;
			return true;
		}
		else {
			prev->unlock();
			curr->unlock();
			return false;
		}

	}
	bool Contains(int key)
	{
	tryAgain:
		NODE* prev, * curr;

		prev = &head;
		curr = prev->next;
		while (curr->key < key) {
			prev = curr;
			curr = curr->next;
		}

		prev->lock();
		curr->lock();

		if (!Validate(prev, curr))
		{
			prev->unlock();
			curr->unlock();
			goto tryAgain;
		}

		if (key == curr->key) {
			prev->unlock();
			curr->unlock();
			return true;
		}
		else {
			prev->unlock();
			curr->unlock();
			return false;
		}
	}
	void PrintTwenty() {
		NODE* prev, * curr;
		prev = &head;
		curr = prev->next;
		int count = 0;

		while (count < 20) {
			cout << curr->key << ", ";
			prev = curr;
			curr = curr->next;
			++count;
		}
		cout << endl;
	}
};

const int MAX_THREADS = 16;
const int NUM_TEST = 400'0000;
const int KEY_RANGE = 1000;
OSET mySet;

void ThreadFunc(int num_thread)
{
	int key;
	for (int i = 0; i < NUM_TEST / num_thread; ++i)
	{
		switch (rand() % 3) {
		case 0: key = rand() % KEY_RANGE;
			mySet.Add(key);
			break;
		case 1: key = rand() % KEY_RANGE;
			mySet.Remove(key);
			break;
		case 2: key = rand() % KEY_RANGE;
			mySet.Contains(key);
			break;
		default: cout << "Error\n";
			exit(-1);
		}
	}
}

int main()
{
	for (int i = 1; i <= MAX_THREADS; i *= 2)
	{
		mySet.Init();

		vector<thread> threads;
		auto start_t = high_resolution_clock::now();

		for (int j = 0; j < i; ++j)
			threads.emplace_back(ThreadFunc, i);

		for (thread& t : threads)
			t.join();

		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;

		mySet.PrintTwenty();
		printf("THREAD_NUM = %d\n", i);
		printf("EXEC TIME = %lld msec\n\n", duration_cast<milliseconds>(exec_t).count());
	}
	return 0;
}