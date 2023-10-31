#include <iostream>
#include <array>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include <limits>
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

게으른 동기화
- Lock 횟수가 비약적으로 감소헀으나 리스트를 두번 순회해야하는 오버헤드가 있다.
- 다시 순회하지 않는 알고리즘을 작성해보자!
	- validate()가 노드를 처음부터 다시 순회하지 않고 validation을 수행한다.
	- pred와 curr의 잠금은 여전히 필요하다.
- Contains()는 Read-Only라 Locking 없이 할 수 있을 것 같다. 이를 Wait-Free로 만들어보자
- 노드에 marked라는 필드를 추가해 노드가 집합에서 제거되어 있는지 표시
	- marked가 true면 제거되었다는 표시
	- marking을 실제 제거보다 반드시 먼저 수행
			- marking은 잠금을 획득한 수만큼 수행된다.
	- 순회를 할 때, 대상 노드를 잠글 필요가 없고 노드가 head에서 접근할 수 있는지 확인하기 위해
		전체리스트를 다시 순회하지 않아도 된다.
- Blocking 알고리즘이라 Convoying이 일어난다.(한 스레드가 Lock을 얻은채 지연되면 다른 스레드도 같이 지연된다.)
- Flag 사용시 메모리 업데이트 순서가 중요해 volatile과 atomic_thread_fence를 적절히 사용하거나
	atomic memory를 사용해야한다.
- 메모리 Leak을 해결해야한다.
	- Delete를 사용하면 오동작이 일어난다.
	- Delete를 하지 않고 모아놓았다가 재사용한다!
		- 아무도 remove된 node를 가리키지 않을 때
		- remove 시점에서 중복실행 중인 모든 method의 호출이 종료되었을 때
	- C++11의 shared_ptr을 사용해 해결해보자

shared_ptr를 사용한 게으른 동기화
- shard_ptr 객체를 load, store하는 것이 atomic이 아니다.
	- shared_ptr<SPNODE> curr = prev->next;		// DATA RACE
- 공유하는 shared_ptr 객체의 load, store를 atomic하게 수행하게 한다.
	- shared_ptr<SPNODE> curr = atomic_load(&prev->next);
	- atomic_exchange(&prev->next, new_node);
- atomic_load와 atomic_exchange는 하나의 lock으로 구현되어 있어 비효율적이다.
- shared_ptr 각각이 별도의 lock을 갖도록 atomic_shared_ptr를 정의해서 사용해보자
- atomic_shared_ptr의 access는 느리다.
	- mutex를 사용하기 때문에 blocking 알고리즘이다.
	- 세밀한 동기화와 다를 것이 없다.
	- 효율적인 atomic_shared_ptr이 필요하다!
		- non-blocking으로 구현!!

Lock-free 알고리즘으로 성능을 향상시키자
- 여러개의 쓰레드에서 동시에 호출했을 때에도 정해진 단위 시간마다 적어도 
	한 개의 호출이 완료되는 알고리즘
- 멀티쓰레드에서 동시에 호출해도 정확한 결과를 만들어주는 알고리즘
- 호출이 다른 쓰레드와 충돌하더라도 적어도 하나의 승자가 있어 승자는 Delay없이 완료됨

Wait-free 알고리즘은?
- 호출이 다른 쓰레드와 충돌해도 모두 Delay없이 완료

추가 상식
- Lock을 사용하지 않는다고 lock-free 알고리즘이 아니다.
- Lock을 사용하면 무조건 lock-free 알고리즘이 아니다.

Non-Blocking은 게으른 동기화에서 시작해
- CAS로 atomic하게 Validation을 검사하고 수정한다.
- 실패하면 다시 검색한다.
- 단, CAS로 Validation을 검사하려면 marking이 false이고
	prev의 next가 curr인지 확인해야한다.
- 그런데? CAS는 한번에 하나의 변수만 바꿀 수 있다!
	- 그렇다면 한 장소에 주소와 Marking을 동시에 저장하자
- x86 CPU는 64bit 중 주소를 저장할 때,
	왼쪽에서 16bit, 오른쪽에서 2bit를 사용하지 않는다.
- 그렇다면 LSB를 marked 변수로 사용하자
	- 하지만 데이터가 망가진다

그럼 Lock-free 알고리즘을 우리 프로젝트에서 쓸 수 있느냐?
- 안된다! 메모리 관리가 제대로 되고 있지 않다 ㅠ
- 그럼 atomic share_ptr은?
	- 느리다. blocking 구현이기도 하다.
	- 지역 변수는 atomic이 아닌 shared_ptr를 사용한다
	- 함수의 parameter에는 const reference를 사용한다.
	- Pointer의 update가 자주 발생하는 경우 shared_ptr로 사용하지 않는다.
- C++20에서부터 atomic_shared_ptr가 지원된다.
- 재사용을 위해서는?
	- Free-List를 사용한다.
	- shared_ptr를 사용한다.

메모리 릭 해결?

Lock-free List 사용
- Free List(자료구조 시간에 배우는 메모리 재사용 리스트)
- NonBlocking에서 아용하기 위해서는 NonBlocking Free List로 구현해야한다.
	- CAS를 사용해서 구현할 수 있다.
- Free List에 있는 Node 재사용
	- 재사용하지 않으면 Memory Leak과 다를 바 없다
	- 제한 없이 재사용하면?
		- Remove()에서 넣은 것을 Add()에서 다시 사용
		- Free List에 넣지 않고 그냥 delete하는 것과 같다.
		- 알고리즘이 오동작 : 검색에서 밟고 지나가는 노드가 다른 곳에 가서 붙는다
		- 안전하게 재사용 하려면 모든 쓰레드가 종료했을 때 재사용하면 된다.
		- 실제 게임에서는 모든 쓰레드의 종료는 게임이 종료했을 때이다.
			- 쓰레드 생성/소멸 오버헤드가 크기 때문에 게임 실행 중에는 쓰레드를 생성하지 않는다.
		- 실제 게임에서는 이러한 Lock-Free 재활용 리스트를 사용할 수 없다.

Atomic shared_ptr
- Blocking이다
- LFNODE에서 사용할 수 없다.
- 느리다
- 판매하는 사용버전의 lock-free atomic_shared_ptr 사용한다.
- 선배가 구현한 lock_free atomic_shared_ptr 사용한다.
	- 검증이 필요
stamped pointer - LF QUEUE 챕터에서 다룸

EPOCH (Epoch Based Reuse) - Lock Free Free-List
- 쓰레드가 종료하지 않아도 재사용 가능 여부를 판단할 수 있게한다.
- Remove된 노드를 Access 하는 포인터가 모든 쓰레드에서 존재하지 않는다.
- Remove되는 순간에 다른 쓰레드들에서 실행중인 메소드들이 다 종료하면, 
	Remove된 Node를 가리키는 포인터는 존재하지 않는다.
	- Remove 되는 Node에 현재 시간을 저장하고, 모든 쓰레드에서 메소드들의 시작시간과 종료
		시간을 적으면 판단이 가능하다.

- 모든 공유 객체는 Epoch Counter와 Thread Epoch Counter[]를 갖고있다.
	- Chrono는 정밀하지 않고 오버헤드가 크고 데이터 크기가 크기 때문에 쓰지 않는다.
- Mothod 호출시
	- EPOCH Counter를 증가
	- Thread Epoch Counter[my_thread_id] = EPOCH Counter
	- 메소드 실행
	- Thread Epoch Counter[my_thread_id] = 0
- 각 쓰레드들은 자신만의 memory pool(free list)을 관리한다.
- remove된 노드를 자신의 memory pool에 MAX(Thread Epoch Counter[])와 같이 넣는다.
- 일정 시간이 지나면 memory pool에 있는 객체들을 실제로 delete 한다.
- Thread Epoch Counter[]의 0이 아닌 최소 값보다 작은 값을 갖고 있는 객체만 delete한다.
- 단점: 어느 한 쓰레드의 Thread Epoch Counter가 증가하지 않는 경우 memory leak과 다름없다.

Hazard Pointer

CAS 없이는 non-blocking 자료구조를 만들 수 없음을 증명
Lock-free Queue의 구현
- ABA 문제
- Stamped Pointer 구현
Lock-free Stack의 구현
- TOP 노드에서의 bottleneck 해소
O(log n) 검색 List 구현
- Lock-free SkipList
- Free List를 통한 Node 재사용의 구현
*/

using namespace std;
using namespace chrono;

constexpr int MAX_THREADS = 32;
constexpr int NUM_TEST = 1000'0000;

class my_mutex
{
public:
	void lock(){}
	void unlock(){}
};

constexpr long long ADDR_MASK = 0xFFFFFFFFFFFFFFFE;
class NODE {
public:
	int key;
	NODE* volatile next;
	volatile bool removed;
	mutex nlock;
	NODE() { key = 1; next = NULL; removed = false; }
	NODE(int key_value) {
		next = NULL;
		removed = false;
		key = key_value;
	}
	~NODE() {}
	void lock() { nlock.lock(); }
	void unlock() { nlock.unlock(); }

};
class SPNODE {
public:
	int key;
	shared_ptr<SPNODE> next;
	volatile bool removed;
	mutex nlock;
	SPNODE() { key = 1; next = NULL; removed = false; }
	SPNODE(int key_value) {
		next = NULL;
		removed = false;
		key = key_value;
	}
	void lock() { nlock.lock(); }
	void unlock() { nlock.unlock(); }
	~SPNODE() {}
};
class LFNODE {
private:
	LFNODE* volatile next;

	bool CAS(long long oldValue, long long newValue)
	{
		return atomic_compare_exchange_strong(
			reinterpret_cast<volatile atomic_llong*>(&next),
			&oldValue, 
			newValue);
	}
public:
	int key;

	LFNODE() { key = -1; next = NULL; }
	LFNODE(int key_value) {
		key = key_value;
		next = NULL;
	}
	~LFNODE() {}
	void SetNext(LFNODE* ptrNext)
	{
		next = ptrNext;
	}
	LFNODE* GetNext() {
		long long ptr = reinterpret_cast<long long>(next) & ADDR_MASK;
		return reinterpret_cast<LFNODE*>(ptr);
	}
	LFNODE* GetNext(bool* removed) {
		long long llNext = reinterpret_cast<long long>(next);
		*removed = ((llNext & 1) == 1);
		return reinterpret_cast<LFNODE*>(llNext & ADDR_MASK);
	}
	bool GetRemoved() {
		long long llNext = reinterpret_cast<long long>(next);
		return ((llNext & 1) == 1);
	}
	bool CAS(LFNODE* oldNode, LFNODE* newNode, bool oldMark, bool newMark)
	{
		long long oldValue = reinterpret_cast<long long>(oldNode);
		long long newValue = reinterpret_cast<long long>(newNode);
		if (oldMark) oldValue = oldValue | 1;
		if (newMark) newValue = newValue | 1;

		return CAS(oldValue, newValue);
	}
};

template<class T>
struct atomic_shared_ptr {
private:
	mutable mutex m_lock;
	shared_ptr<T> m_ptr;
public:
	bool is_lock_free() const noexcept
	{
		return false;
	}
	void store(shared_ptr<T> sptr, memory_order = memory_order_seq_cst) noexcept
	{
		m_lock.lock();
		m_ptr = sptr;
		m_lock.unlock();
	}
	shared_ptr<T> load(memory_order = memory_order_seq_cst) const noexcept
	{
		m_lock.lock();
		shared_ptr<T> t = m_ptr;
		m_lock.unlock();
		return t;
	}
	operator shared_ptr<T>() const noexcept
	{
		m_lock.lock();
		shared_ptr<T> t = m_ptr;
		m_lock.unlock();
		return t;
	}
	shared_ptr<T> exchange(shared_ptr<T> sptr, memory_order = memory_order_seq_cst) noexcept
	{
		m_lock.lock();
		shared_ptr<T> t = m_ptr;
		m_ptr = sptr;
		m_lock.unlock();
		return t;
	}
	bool compare_exchange_strong(shared_ptr<T>& expected_sptr, shared_ptr<T> new_sptr, memory_order, memory_order) noexcept
	{
		bool success = false;
		m_lock.lock();
		shared_ptr<T> t = m_ptr;
		if (m_ptr.get() == expected_sptr.get()) {
			m_ptr = new_sptr;
			success = true;
		}
		expected_sptr = m_ptr;
		m_lock.unlock();
	}
	bool compare_exchange_weak(shared_ptr<T>& expected_sptr, shared_ptr<T> new_sptr, memory_order, memory_order) noexcept
	{
		return compare_exchange_strong(expected_sptr, new_sptr, memory_order);
	}
	atomic_shared_ptr() noexcept = default;
	constexpr atomic_shared_ptr(shared_ptr<T> sptr) noexcept
	{
		m_lock.lock();
		m_ptr = sptr;
		m_lock.unlock();
	}
	shared_ptr<T> operator=(shared_ptr<T> sptr) noexcept
	{
		m_lock.lock();
		m_ptr = sptr;
		m_lock.unlock();
		return sptr;
	}
	void reset()
	{
		m_lock.lock();
		m_ptr = nullptr;
		m_lock.unlock();
	}
	atomic_shared_ptr& operator=(const atomic_shared_ptr& rhs)
	{
		store(rhs);
		return *this;
	}
	shared_ptr<T>& operator->() {
		std::lock_guard<mutex> tt(m_lock);
		return m_ptr;
	}
	template<typename TargetType>
	inline bool operator ==(shared_ptr<TargetType> const& rhs)
	{
		return load() == rhs;
	}
	template<typename TargetType>
	inline bool operator ==(atomic_shared_ptr<TargetType> const& rhs)
	{
		lock_guard<mutex> t1(m_lock);
		lock_guard<mutex> t2(rhs.m_lock);
		return m_ptr == rhs.m_ptr;
	}
};
class ASPNODE {
	mutex n_lock;
public:
	int key;
	atomic_shared_ptr <ASPNODE> next;
	volatile bool removed;
	ASPNODE() : key(-1), next(nullptr), removed(false) {}
	ASPNODE(int x) : key(x), next(nullptr), removed(false) {}
	void lock()
	{
		n_lock.lock();
	}
	void unlock()
	{
		n_lock.unlock();
	}
};
class ASPNODE2 {
	mutex n_lock;
public:
	int key;
	atomic<shared_ptr<ASPNODE2>> next;
	volatile bool removed;
	ASPNODE2() : key(-1), next(nullptr), removed(false) {}
	ASPNODE2(int x) : key(x), next(nullptr), removed(false) {}
	void lock()
	{
		n_lock.lock();
	}
	void unlock()
	{
		n_lock.unlock();
	}
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
		NODE* prev, * curr;
	tryAgain:
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
		NODE* prev, * curr;
	tryAgain:
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

	tryAgain:
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
class LSET {
	NODE head, tail;
public:
	LSET()
	{
		head.key = numeric_limits<int>::min();
		tail.key = numeric_limits<int>::max();
		head.next = &tail;
	}
	~LSET() { Init(); }

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
		return (prev->removed == false) && (curr->removed == false) && (prev->next == curr);
	}
	bool Add(int key)
	{
		while (true)
		{
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
				return false;
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
	}
	bool Remove(int key)
	{
		while (true)
		{
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
				return false;
			}

			if (key == curr->key) {
				curr->removed = true;
				prev->next = curr->next;
				prev->unlock();
				curr->unlock();
				//delete curr;
				return true;
			}
			else {
				prev->unlock();
				curr->unlock();
				return false;
			}
		}
	}
	bool Contains(int key)
	{
		NODE* curr;
		curr = head.next;
		while (curr->key < key) {
			curr = curr->next;
		}

		return curr->key == key && !curr->removed;
	}
	void PrintTwenty() {
		NODE* prev, * curr;
		prev = &head;
		curr = prev->next;
		int count = 0;

		while (count < 20) {
			if (curr == &tail)
				break;
			cout << curr->key << ", ";
			prev = curr;
			curr = curr->next;
			++count;
		}
		cout << endl;
	}
};
#if NOTC20
class LSPSET {
	shared_ptr<SPNODE> head, tail;
public:
	LSPSET()
	{
		head = make_shared<SPNODE>(numeric_limits<int>::min());
		tail = make_shared<SPNODE>(numeric_limits<int>::max());
		head->next = tail;
	}
	~LSPSET() { }

	void Init()
	{
		head->next = tail;
	}
	bool Validate(const shared_ptr<SPNODE>& prev, const shared_ptr<SPNODE>& curr)
	{
		return (prev->removed == false) && (curr->removed == false) && (prev->next == curr);
	}
	bool Add(int key)
	{
		while (true)
		{
			shared_ptr<SPNODE> prev, curr;

			prev = head;
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
				return false;
			}

			if (key == curr->key) {
				prev->unlock();
				curr->unlock();
				return false;
			}
			else {
				shared_ptr<SPNODE> node = make_shared<SPNODE>(key);	// 이 또한 빼면 좋다
				node->next = curr;
				prev->next = node;
				prev->unlock();
				curr->unlock();
				return true;
			}
		}
	}
	bool Remove(int key)
	{
		while (true)
		{
			shared_ptr<SPNODE> prev, curr;
			prev = head;
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
				return false;
			}

			if (key == curr->key) {
				curr->removed = true;
				prev->next = curr->next;
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
	}
	bool Contains(int key)
	{
		shared_ptr<SPNODE> curr;
		curr = head->next;
		while (curr->key < key) {
			curr = curr->next;
		}

		return curr->key == key && !curr->removed;
	}
	void PrintTwenty() {
		shared_ptr<SPNODE> prev, curr;
		prev = head;
		curr = prev->next;
		int count = 0;

		while (count < 20) {
			cout << curr->key << ", ";
			prev = curr;
			curr = curr->next;
			if (curr == tail)
				break;
			++count;
		}
		cout << endl;
	}
};
class LASPSET {
	shared_ptr<SPNODE> head, tail;
public:
	LASPSET()
	{
		head = make_shared<SPNODE>(numeric_limits<int>::min());
		tail = make_shared<SPNODE>(numeric_limits<int>::max());
		head->next = tail;
	}
	~LASPSET() { }

	void Init()
	{
		head->next = tail;
	}
	bool Validate(const shared_ptr<SPNODE>& prev, const shared_ptr<SPNODE>& curr)
	{
		return (prev->removed == false) && (curr->removed == false) && (prev->next == curr);
	}
	bool Add(int key)
	{
		while (true)
		{
			shared_ptr<SPNODE> prev, curr;

			prev = head;
			curr = atomic_load(&prev->next);
			while (curr->key < key) {
				prev = curr;
				curr = atomic_load(&curr->next);
			}

			prev->lock();
			curr->lock();

			if (!Validate(prev, curr))
			{
				prev->unlock();
				curr->unlock();
				return false;
			}

			if (key == curr->key) {
				prev->unlock();
				curr->unlock();
				return false;
			}
			else {
				shared_ptr<SPNODE> node = make_shared<SPNODE>(key);	// 이 또한 빼면 좋다
				node->next = curr;
				atomic_exchange(&prev->next, node);
				prev->unlock();
				curr->unlock();
				return true;
			}
		}
	}
	bool Remove(int key)
	{
		while (true)
		{
			shared_ptr<SPNODE> prev, curr;
			prev = head;
			curr = atomic_load(&prev->next);
			while (curr->key < key) {
				prev = curr;
				curr = atomic_load(&curr->next);
			}

			prev->lock();
			curr->lock();

			if (!Validate(prev, curr))
			{
				prev->unlock();
				curr->unlock();
				return false;
			}

			if (key == curr->key) {
				curr->removed = true;
				atomic_exchange(&prev->next, curr->next);
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
	}
	bool Contains(int key)
	{
		shared_ptr<SPNODE> curr;
		curr = head->next;
		while (curr->key < key) {
			curr = atomic_load(&curr->next);
		}

		return curr->key == key && !curr->removed;
	}
	void PrintTwenty() {
		shared_ptr<SPNODE> prev, curr;
		prev = head;
		curr = prev->next;
		int count = 0;

		while (count < 20) {
			cout << curr->key << ", ";
			prev = curr;
			curr = curr->next;
			if (curr == tail)
				break;
			++count;
		}
		cout << endl;
	}
};
#endif
class LASPSET2 {
	atomic_shared_ptr<ASPNODE> head, tail;
public:
	LASPSET2()
	{
		head = make_shared<ASPNODE>(numeric_limits<int>::min());
		tail = make_shared<ASPNODE>(numeric_limits<int>::max());
		head->next = tail;
	}
	~LASPSET2() { }

	void Init()
	{
		head->next = tail;
	}
	bool Validate(const shared_ptr<ASPNODE>& prev, const shared_ptr<ASPNODE>& curr)
	{
		return (prev->removed == false) && (curr->removed == false) && (prev->next == curr);
	}
	bool Add(int key)
	{
		while (true)
		{
			shared_ptr<ASPNODE> prev, curr;

			prev = head;
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
				return false;
			}

			if (key == curr->key) {
				prev->unlock();
				curr->unlock();
				return false;
			}
			else {
				shared_ptr<ASPNODE> node = make_shared<ASPNODE>(key);	// 이 또한 빼면 좋다
				node->next = curr;
				prev->next = node;
				prev->unlock();
				curr->unlock();
				return true;
			}
		}
	}
	bool Remove(int key)
	{
		while (true)
		{
			shared_ptr<ASPNODE> prev, curr;
			prev = head;
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
				return false;
			}

			if (key == curr->key) {
				curr->removed = true;
				prev->next = curr->next;
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
	}
	bool Contains(int key)
	{
		shared_ptr<ASPNODE> curr;
		curr = head->next;
		while (curr->key < key) {
			curr = curr->next;
		}

		return curr->key == key && !curr->removed;
	}
	void PrintTwenty() {
		shared_ptr<ASPNODE> prev, curr;
		prev = head;
		curr = prev->next;
		int count = 0;

		while (count < 20) {
			cout << curr->key << ", ";
			prev = curr;
			curr = curr->next;
			if (curr == shared_ptr<ASPNODE>{ tail })
				break;
			++count;
		}
		cout << endl;
	}
};
class LFSET {
	LFNODE head, tail;
public:
	LFSET()
	{
		head.key = numeric_limits<int>::min();
		tail.key = numeric_limits<int>::max();
		head.SetNext(&tail);
	}
	~LFSET() { Init(); }

	void Init()
	{
		LFNODE* ptr;
		while (head.GetNext() != &tail) {
			ptr = head.GetNext();
			head.SetNext(ptr->GetNext());
			delete ptr;
		}
	}
	void Find(LFNODE* &prev, LFNODE* &curr, int key)
	{
	retry:
		prev = &head;
		curr = prev->GetNext();
		while (true)
		{
			bool removed;
			LFNODE* succ = curr->GetNext(&removed);
			while(removed)
			{
				if (prev->CAS(curr, succ, false, false) == false)
					goto retry;
				curr = succ;
				succ = curr->GetNext(&removed);
			}

			if (curr->key >= key)
				return;

			prev = curr;
			curr = succ;
		}
	}
	bool Add(int key)
	{
		LFNODE* node = new LFNODE{ key };
		while (true)
		{
			LFNODE* prev, * curr;

			Find(prev, curr, key);

			if (key == curr->key) {
				delete node;
				return false;
			}
			else {
				node->SetNext(curr);
				if (prev->CAS(curr, node, false, false))
					return true;
			}
		}
	}
	bool Remove(int key)
	{
		while (true) {
			LFNODE* prev = &head;
			LFNODE* curr = prev->GetNext();
			Find(prev, curr,key);

			if (curr->key != key) {
				return false;
			}
			else
			{
				LFNODE* succ = curr->GetNext();

				if (false == curr->CAS(succ, succ, false, true))
					continue;

				prev->CAS(curr, succ, false, false);
				return true;
			}
		}
		return true;
		return false;
	}
	bool Contains(int key)
	{
		LFNODE* prev, * curr;
		Find(prev, curr, key);

		if (key == curr->key) {
			return !curr->GetRemoved();
		}
		return false;
	}
	void PrintTwenty() {
		LFNODE* prev, * curr;
		prev = &head;
		curr = prev->GetNext();
		int count = 0;

		while (count < 20) {
			if (curr == &tail)
				break;

			cout << curr->key << ", ";
			prev = curr;
			curr = curr->GetNext();
			++count;
		}
		cout << endl;
	}
};

//C++20의 atomic shared_ptr 사용
class LASPSET3 {
	atomic<shared_ptr<ASPNODE2>> head, tail;
public:
	LASPSET3()
	{
		head = make_shared<ASPNODE2>(numeric_limits<int>::min());
		tail = make_shared<ASPNODE2>(numeric_limits<int>::max());
		shared_ptr<ASPNODE2> h = head;
		shared_ptr<ASPNODE2> t = tail;
		h->next = t;
	}
	~LASPSET3() { }

	void Init()
	{
		shared_ptr<ASPNODE2> h = head;
		shared_ptr<ASPNODE2> t = tail;
		h->next = t;
	}
	bool Validate(const shared_ptr<ASPNODE2>& prev, const shared_ptr<ASPNODE2>& curr)
	{
		shared_ptr<ASPNODE2> p = prev->next;
		return (prev->removed == false) && (curr->removed == false) && (p == curr);
	}
	bool Add(int key)
	{
		while (true)
		{
			shared_ptr<ASPNODE2> prev, curr;

			prev = head;
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
				return false;
			}

			if (key == curr->key) {
				prev->unlock();
				curr->unlock();
				return false;
			}
			else {
				shared_ptr<ASPNODE2> node = make_shared<ASPNODE2>(key);	// 이 또한 빼면 좋다
				node->next = curr;
				prev->next = node;
				prev->unlock();
				curr->unlock();
				return true;
			}
		}
	}
	bool Remove(int key)
	{
		while (true)
		{
			shared_ptr<ASPNODE2> prev, curr;
			prev = head;
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
				return false;
			}

			if (key == curr->key) {
				curr->removed = true;

				shared_ptr<ASPNODE2> p1 = prev;
				shared_ptr<ASPNODE2> p2 = curr->next;

				p1 = p2;
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
	}
	bool Contains(int key)
	{
		shared_ptr<ASPNODE2> curr = head;

		while (curr->key < key) {
			curr = curr->next;
		}

		return curr->key == key && !curr->removed;
	}
	void PrintTwenty() {
		shared_ptr<ASPNODE2> prev, curr;
		prev = head;
		curr = prev->next;
		int count = 0;

		while (count < 20) {
			cout << curr->key << ", ";
			prev = curr;
			curr = curr->next;
			if (curr == shared_ptr<ASPNODE2>{ tail })
				break;
			++count;
		}
		cout << endl;
	}
};

// Lock Free 버전의 게으른 동기화 구현
// Eclass 파일을 찾지 못함;



constexpr int MAX_THREADS = 32;
constexpr int NUM_TEST = 4000000;
constexpr int KEY_RANGE = 1000;
LASPSET3 mySet;

class HISTORY {
public:
	int op;
	int i_value;
	bool o_value;
	HISTORY(int o, int i, bool re) : op(o), i_value(i), o_value(re) {}
};

void ThreadFunc(vector<HISTORY>* history, int num_thread)
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

void ThreadFunc_Check(vector<HISTORY>* history, int num_threads)
{
	for (int i = 0; i < NUM_TEST / num_threads; ++i) {
		int op = rand() % 3;
		switch (op) {
		case 0: {
			int v = rand() % KEY_RANGE;
			history->emplace_back(0, v, mySet.Add(v));
			break;
		}
		case 1: {
			int v = rand() % KEY_RANGE;
			history->emplace_back(1, v, mySet.Remove(v));
			break;
		}
		case 2: {
			int v = rand() % KEY_RANGE;
			history->emplace_back(2, v, mySet.Contains(v));
			break;
		}
		}
	}
}

void Check_History(array <vector <HISTORY>, MAX_THREADS>& history, int num_threads)
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
			if (mySet.Contains(i)) {
				cout << "ERROR. The value " << i << " should not exists.\n";
				exit(-1);
			}
		}
		else if (val == 1) {
			if (false == mySet.Contains(i)) {
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
		mySet.Init();

		vector<thread> threads;
		array<vector <HISTORY>, MAX_THREADS> history;
		auto start_t = high_resolution_clock::now();

		for (int j = 0; j < i; ++j)
			threads.emplace_back(ThreadFunc_Check, &history[j], i);

		for (thread& t : threads)
			t.join();

		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;

		mySet.PrintTwenty();
		printf("THREAD_NUM = %d\n", i);
		printf("EXEC TIME = %lld msec\n", duration_cast<milliseconds>(exec_t).count());
		Check_History(history, i);
		printf("\n");
	}

	cout << "======== SPEED CHECK =============\n";

	for (int i = 1; i <= MAX_THREADS; i *= 2)
	{
		mySet.Init();

		vector<thread> threads;
		array<vector <HISTORY>, MAX_THREADS> history;
		auto start_t = high_resolution_clock::now();

		for (int j = 0; j < i; ++j)
			threads.emplace_back(ThreadFunc, &history[j], i);

		for (thread& t : threads)
			t.join();

		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;

		mySet.PrintTwenty();
		printf("THREAD_NUM = %d\n", i);
		printf("EXEC TIME = %lld msec\n", duration_cast<milliseconds>(exec_t).count());
		Check_History(history, i);
		printf("\n");
	}
	return 0;
}