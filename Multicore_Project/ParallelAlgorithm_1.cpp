#include <iostream>
#include <array>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include <limits>
/*
Non Blocking �ڷ� ������ �����غ���
���������� Lock-Free �ڷᱸ���� �����Ѵ�.

Set�� Non Blocking �ڷᱸ���� �����غ���.

field
- key: ����Ʈ�� ���� �Ǵ� ��
- next: ���� ����� ������

method
- add(x): ���տ� x�� �߰��ϰ� ������ true�� ��ȯ
- remove(x): ���տ��� x�� �����ϰ� ������ true�� ��ȯ
- contains(x): ���տ� x�� ������ true�� ��ȯ

- �˻��� ȿ������ ���� MAX_INT, -MAX_INT ���� ������ 
	���� ��� Head, Tail�� �߰��Ѵ�.

���� ����
- ���� ����ȭ(Lock �ϳ��� ����ȭ ��ü ��ü�� ���δ� ���)
- ������ ����ȭ
- ��õ���� ����ȭ
- ������ ����ȭ
- ����� ����ȭ

���� ����ȭ
- ����Ʈ�� �ϳ��� Lock�� ������ ������ ��� method ȣ����
	�� Lock�� �̿��� Critical Section���� ����
- ������ ���� ��� ���� ���������� ������ ������ ���� ������ ����
- Blocking �˰���

new�� delete�� ���������� locking�� ����ϱ� ������ 
��Ƽ������ ���α׷��ֿ����� �� ������� �ʴ´�.

Lock, Unlock�� ����ϸ� ���༺�� ���ѵȴ�.
���༺�� ����ų �� �ִ� ����� ã�ƺ���.

������ ����ȭ
- ��ü ����Ʈ�� �Ѳ����� ��״� �ͺ��� ���� ��带 �ᰡ ���༺�� ����Ų��.

��忡 Mutex�� �ְ� ���� Lock�� ����.
Head ���� Node �̵��� �� �� Lock�� ��׸鼭 �̵��Ѵ�.

Lock/Unlock Ƚ���� �ʹ� ���� ������ ���� �ʴ�.
������ ���ļ��� ������ �� ���� �������� ����� ������ �� �ִ�.
head�� bottlelack ������ ������ ������ �����̻󿡼� �����ص� �������� �ʴ´�.

�׷��ٸ� Lock/Unlock�� Ƚ���� ��� ���� �� ������?
- �̵��� �� ����� ���� �ʴ´�!!
- Add/Remove�� ���� pred�� �����ϱ� ���� pred�� ��ٴ�.

��õ�� ����ȭ
- �̵��� ����� ���� �ʴ´�.
- ������ �ܼ��� ����� ���� ������ ���α׷��� �״´�.
- Crash(�Ǵ� ���ѷ���)�� ��� ���ŵ� Node�� next�� �ٸ� ������ ���� �Ǽ� ������ �����.
	- ���ŵ� Node�� next�� crash�� �߻���Ű�� ���� ���� �ʰ� �Ѵ�.
	- ���ŵ� Node�� next�� ���󰡸� Tail�� �������Ѵ�.
- Node�� �̻��� ��ġ�� ���� ������ �������� �Ѵ�.
	- pred�� curr�� �߸� �ᰬ�� ��� ó������ �ٽ� �����Ѵ�.
	- pred�� curr�� �ᰬ�� ��, ����� �ᰬ���� �˻��� Ȯ���Ѵ�.
- ���ŵ� ��带 delete���� �ʴ´�.
	- next �ʵ�� �������� �ʰ� �Ѿư��� Tail�� ������ �ȴ�.
	- ��, �޸𸮰� ����Ǵµ� ���߿� �ذ��Ѵ�.
	- �ڹ� ȯ�濡���� �� �̻� ������ �ʴ� Node�� �ڵ������� �����Ǽ� ������ ����.
- Validation ����
	- ����� pred�� curr�� ���ŵ��� �ʾҰ�
	- pred�� curr���̿� �ٸ� ��尡 ������� �ʾҴ�.
- Validate�� �����ϸ� ó������ �ٽ� �����ϱ� ������ �ٸ� �����尡 ��� ������ ��õ��� �Ұ��
	��Ƹ� ���� �� ������ ��ġ ���� ����̱� ������ �����δ� �� ������ ���ɼ��� ũ��.

������ ����ȭ
- Lock Ƚ���� ��������� ���������� ����Ʈ�� �ι� ��ȸ�ؾ��ϴ� ������尡 �ִ�.
- �ٽ� ��ȸ���� �ʴ� �˰����� �ۼ��غ���!
	- validate()�� ��带 ó������ �ٽ� ��ȸ���� �ʰ� validation�� �����Ѵ�.
	- pred�� curr�� ����� ������ �ʿ��ϴ�.
- Contains()�� Read-Only�� Locking ���� �� �� ���� �� ����. �̸� Wait-Free�� ������
- ��忡 marked��� �ʵ带 �߰��� ��尡 ���տ��� ���ŵǾ� �ִ��� ǥ��
	- marked�� true�� ���ŵǾ��ٴ� ǥ��
	- marking�� ���� ���ź��� �ݵ�� ���� ����
			- marking�� ����� ȹ���� ����ŭ ����ȴ�.
	- ��ȸ�� �� ��, ��� ��带 ��� �ʿ䰡 ���� ��尡 head���� ������ �� �ִ��� Ȯ���ϱ� ����
		��ü����Ʈ�� �ٽ� ��ȸ���� �ʾƵ� �ȴ�.
- Blocking �˰����̶� Convoying�� �Ͼ��.(�� �����尡 Lock�� ����ä �����Ǹ� �ٸ� �����嵵 ���� �����ȴ�.)
- Flag ���� �޸� ������Ʈ ������ �߿��� volatile�� atomic_thread_fence�� ������ ����ϰų�
	atomic memory�� ����ؾ��Ѵ�.
- �޸� Leak�� �ذ��ؾ��Ѵ�.
	- Delete�� ����ϸ� �������� �Ͼ��.
	- Delete�� ���� �ʰ� ��Ƴ��Ҵٰ� �����Ѵ�!
		- �ƹ��� remove�� node�� ����Ű�� ���� ��
		- remove �������� �ߺ����� ���� ��� method�� ȣ���� ����Ǿ��� ��
	- C++11�� shared_ptr�� ����� �ذ��غ���

shared_ptr�� ����� ������ ����ȭ
- shard_ptr ��ü�� load, store�ϴ� ���� atomic�� �ƴϴ�.
	- shared_ptr<SPNODE> curr = prev->next;		// DATA RACE
- �����ϴ� shared_ptr ��ü�� load, store�� atomic�ϰ� �����ϰ� �Ѵ�.
	- shared_ptr<SPNODE> curr = atomic_load(&prev->next);
	- atomic_exchange(&prev->next, new_node);
- atomic_load�� atomic_exchange�� �ϳ��� lock���� �����Ǿ� �־� ��ȿ�����̴�.
- shared_ptr ������ ������ lock�� ������ atomic_shared_ptr�� �����ؼ� ����غ���
- atomic_shared_ptr�� access�� ������.
	- mutex�� ����ϱ� ������ blocking �˰����̴�.
	- ������ ����ȭ�� �ٸ� ���� ����.
	- ȿ������ atomic_shared_ptr�� �ʿ��ϴ�!
		- non-blocking���� ����!!

Lock-free �˰������� ������ ����Ű��
- �������� �����忡�� ���ÿ� ȣ������ ������ ������ ���� �ð����� ��� 
	�� ���� ȣ���� �Ϸ�Ǵ� �˰���
- ��Ƽ�����忡�� ���ÿ� ȣ���ص� ��Ȯ�� ����� ������ִ� �˰���
- ȣ���� �ٸ� ������� �浹�ϴ��� ��� �ϳ��� ���ڰ� �־� ���ڴ� Delay���� �Ϸ��

Wait-free �˰�����?
- ȣ���� �ٸ� ������� �浹�ص� ��� Delay���� �Ϸ�

�߰� ���
- Lock�� ������� �ʴ´ٰ� lock-free �˰����� �ƴϴ�.
- Lock�� ����ϸ� ������ lock-free �˰����� �ƴϴ�.

Non-Blocking�� ������ ����ȭ���� ������
- CAS�� atomic�ϰ� Validation�� �˻��ϰ� �����Ѵ�.
- �����ϸ� �ٽ� �˻��Ѵ�.
- ��, CAS�� Validation�� �˻��Ϸ��� marking�� false�̰�
	prev�� next�� curr���� Ȯ���ؾ��Ѵ�.
- �׷���? CAS�� �ѹ��� �ϳ��� ������ �ٲ� �� �ִ�!
	- �׷��ٸ� �� ��ҿ� �ּҿ� Marking�� ���ÿ� ��������
- x86 CPU�� 64bit �� �ּҸ� ������ ��,
	���ʿ��� 16bit, �����ʿ��� 2bit�� ������� �ʴ´�.
- �׷��ٸ� LSB�� marked ������ �������
	- ������ �����Ͱ� ��������

�׷� Lock-free �˰����� �츮 ������Ʈ���� �� �� �ִ���?
- �ȵȴ�! �޸� ������ ����� �ǰ� ���� �ʴ� ��
- �׷� atomic share_ptr��?
	- ������. blocking �����̱⵵ �ϴ�.
	- ���� ������ atomic�� �ƴ� shared_ptr�� ����Ѵ�
	- �Լ��� parameter���� const reference�� ����Ѵ�.
	- Pointer�� update�� ���� �߻��ϴ� ��� shared_ptr�� ������� �ʴ´�.
- C++20�������� atomic_shared_ptr�� �����ȴ�.
- ������ ���ؼ���?
	- Free-List�� ����Ѵ�.
	- shared_ptr�� ����Ѵ�.

�޸� �� �ذ�?

Lock-free List ���
- Free List(�ڷᱸ�� �ð��� ���� �޸� ���� ����Ʈ)
- NonBlocking���� �ƿ��ϱ� ���ؼ��� NonBlocking Free List�� �����ؾ��Ѵ�.
	- CAS�� ����ؼ� ������ �� �ִ�.
- Free List�� �ִ� Node ����
	- �������� ������ Memory Leak�� �ٸ� �� ����
	- ���� ���� �����ϸ�?
		- Remove()���� ���� ���� Add()���� �ٽ� ���
		- Free List�� ���� �ʰ� �׳� delete�ϴ� �Ͱ� ����.
		- �˰����� ������ : �˻����� ��� �������� ��尡 �ٸ� ���� ���� �ٴ´�
		- �����ϰ� ���� �Ϸ��� ��� �����尡 �������� �� �����ϸ� �ȴ�.
		- ���� ���ӿ����� ��� �������� ����� ������ �������� ���̴�.
			- ������ ����/�Ҹ� ������尡 ũ�� ������ ���� ���� �߿��� �����带 �������� �ʴ´�.
		- ���� ���ӿ����� �̷��� Lock-Free ��Ȱ�� ����Ʈ�� ����� �� ����.

Atomic shared_ptr
- Blocking�̴�
- LFNODE���� ����� �� ����.
- ������
- �Ǹ��ϴ� �������� lock-free atomic_shared_ptr ����Ѵ�.
- ���谡 ������ lock_free atomic_shared_ptr ����Ѵ�.
	- ������ �ʿ�
stamped pointer - LF QUEUE é�Ϳ��� �ٷ�

EPOCH (Epoch Based Reuse) - Lock Free Free-List
- �����尡 �������� �ʾƵ� ���� ���� ���θ� �Ǵ��� �� �ְ��Ѵ�.
- Remove�� ��带 Access �ϴ� �����Ͱ� ��� �����忡�� �������� �ʴ´�.
- Remove�Ǵ� ������ �ٸ� ������鿡�� �������� �޼ҵ���� �� �����ϸ�, 
	Remove�� Node�� ����Ű�� �����ʹ� �������� �ʴ´�.
	- Remove �Ǵ� Node�� ���� �ð��� �����ϰ�, ��� �����忡�� �޼ҵ���� ���۽ð��� ����
		�ð��� ������ �Ǵ��� �����ϴ�.

- ��� ���� ��ü�� Epoch Counter�� Thread Epoch Counter[]�� �����ִ�.
	- Chrono�� �������� �ʰ� ������尡 ũ�� ������ ũ�Ⱑ ũ�� ������ ���� �ʴ´�.
- Mothod ȣ���
	- EPOCH Counter�� ����
	- Thread Epoch Counter[my_thread_id] = EPOCH Counter
	- �޼ҵ� ����
	- Thread Epoch Counter[my_thread_id] = 0
- �� ��������� �ڽŸ��� memory pool(free list)�� �����Ѵ�.
- remove�� ��带 �ڽ��� memory pool�� MAX(Thread Epoch Counter[])�� ���� �ִ´�.
- ���� �ð��� ������ memory pool�� �ִ� ��ü���� ������ delete �Ѵ�.
- Thread Epoch Counter[]�� 0�� �ƴ� �ּ� ������ ���� ���� ���� �ִ� ��ü�� delete�Ѵ�.
- ����: ��� �� �������� Thread Epoch Counter�� �������� �ʴ� ��� memory leak�� �ٸ�����.

Hazard Pointer

CAS ���̴� non-blocking �ڷᱸ���� ���� �� ������ ����
Lock-free Queue�� ����
- ABA ����
- Stamped Pointer ����
Lock-free Stack�� ����
- TOP ��忡���� bottleneck �ؼ�
O(log n) �˻� List ����
- Lock-free SkipList
- Free List�� ���� Node ������ ����
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
			NODE* node = new NODE(key);	// �� ���� ���� ����
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
			delete curr;	// Unlock()�� �� �ڿ� delete�ϴ� ���� ����. delete ����� �ü���� ȣ�⶧��
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
			NODE* node = new NODE(key);	// �� ���� ���� ����
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
			NODE* node = new NODE(key);	// �� ���� ���� ����
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
				NODE* node = new NODE(key);	// �� ���� ���� ����
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
				shared_ptr<SPNODE> node = make_shared<SPNODE>(key);	// �� ���� ���� ����
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
				shared_ptr<SPNODE> node = make_shared<SPNODE>(key);	// �� ���� ���� ����
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
				shared_ptr<ASPNODE> node = make_shared<ASPNODE>(key);	// �� ���� ���� ����
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

//C++20�� atomic shared_ptr ���
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
				shared_ptr<ASPNODE2> node = make_shared<ASPNODE2>(key);	// �� ���� ���� ����
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

// Lock Free ������ ������ ����ȭ ����
// Eclass ������ ã�� ����;



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