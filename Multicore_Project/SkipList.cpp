/*
����� ������������ memory DB�� ���
���� ���� �����ʹ� �޸𸮿� �÷�����
REPIS(no SQL)

SkipList
- ��� O(log n) �˻��ð��� ���� �ڷᱸ��
- ������۾��� �ʿ� ����.
- ���� �ڷᱸ��. (������ ���ſ��� �������� ���ſ��� �𸣰ڰ� �������� �ϰڴ�.)
- �־��� ��� O(n)�̱� ������ �׷����� ���� ����.
- ������ �߰��� ��ŵ�����͸� ������ ����
- �˻��� �� �� ��ŵ�����͸� ���� �˻�

���� ��ŵ����Ʈ
- ������ ������ head����
- ���� ������ �����ͺ��� �˻��� �����ϰ� �� ������ �˻��� ������ ���� ������ �˻�
- �̶�, �������� �˻� ����� ���� (Add�� �� ��, �߰��Ǵ� ��尡 �� ������ �𸣱� ����)
- �� �Ʒ� ������ ������ ��� ����
- ���� ���� ��尡 �� ���� ���⵵�� ����

���� ����ȭ�� ����ϴٺ���, ������ ������ �þ���� ����ӵ��� ��������.
���ļ��� ����� ��ŵ����Ʈ�� �����غ���.

������ ����ȭ ����� ��ŵ����Ʈ�� ����
- ����� �߰������� ������ ����ؾ��Ѵ�. (�ڵ� ���ٸ����� ��尡 �߰����� �ʰ�, ������ ����.)
- ��� ����� �߰��� �Ϸ�Ǿ��� ��, ��尡 �߰��Ǿ��ٰ� ����
- removed�� ���������� fullyLinked field�� �߰��� Ȯ���Ѵ�.
- �Ϲ������δ� locking�� �����尡 �ٽ� locking�� �ϸ� ����
- recursive_mutex�� ����� �ذ�
- find�ÿ� fullyLinked�� �ƴ� ��带 ã���� ���, ����� ã���� �ƴϴ�.

mutex���� Convoying�� �Ͼ�� ������ fully_linked�� Ȯ���ϱ� ���� ��ٸ��� �ڵ�� ����
Convoying�� �Ͼ ����ӵ��� ���� �� �� �ִ�.(�� �ھ�� �����尡 �� ���� ������ ���)

LockFreeSkipList������ ���� CAS�� ���� �ռ� �����͸� ����Ѵ�.
��� ������ next�� ��ŷ�� �Ѵ�. �� ���������� �˻��� �ϸ鼭 ������ �ϱ� ����.
Find()�� ����Ʈ�� ��ȸ�ϸ鼭 ��ŷ�� ��带 ���� ������ �߶󳻾� 
��ŷ�� ��尡 �˻� ����� ���Ե��� �ʰ� �Ѵ�.
0���� ����Ǿ��� ��, �߰��Ǿ��ٰ� ����
=> 0������ ������ �����Ѵ�.	
*/

#include <iostream>
#include <array>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include <limits>

using namespace std;
using namespace chrono;

constexpr int TOP_LEVEL = 10;
constexpr int MAX_THREADS = 32;
constexpr int NUM_TEST = 4000000;
constexpr int KEY_RANGE = 1000;

class my_mutex
{
public:
	void lock() {}
	void unlock() {}
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
class SKNODE {
public:
	int key;
	SKNODE* volatile next[TOP_LEVEL + 1];
	int top_level;
	volatile bool bRemoved;
	volatile bool bFullyLinked;
	recursive_mutex nlock;
	SKNODE() : key(-1), top_level(0), bRemoved(false), bFullyLinked(false) { 
		for (auto& n : next)
			n = nullptr;
	}
	SKNODE(int key_value, int top) : key(key_value), top_level(top), 
		bRemoved(false), bFullyLinked(false) {
		for (auto& n : next)
			n = nullptr;
	}
	~SKNODE() {}
};
class LFSKNODE {
	LFSKNODE* volatile next[TOP_LEVEL + 1];
public:
	int key;
	int top_level;
	LFSKNODE() : key(-1), top_level(0) {
		for (auto& n : next)
			n = nullptr;
	}
	LFSKNODE(int key_value, int top) : key(key_value), top_level(top) {
		for (auto& n : next)
			n = nullptr;
	}
	void Set(int level, LFSKNODE* ptr, bool bRemoved) {
		long long temp = reinterpret_cast<long long>(ptr);
		if (bRemoved) temp = temp | 1;
		next[level] = reinterpret_cast<LFSKNODE*>(temp);
	}
	LFSKNODE* Get(int level, bool* bRemoved) {
		long long temp = reinterpret_cast<long long>(next[level]);
		*bRemoved = ((temp & 1) == 1);
		return reinterpret_cast<LFSKNODE*>(temp & ADDR_MASK);
	}
	LFSKNODE* Get(int level) {
		long long temp = reinterpret_cast<long long>(next[level]);
		return reinterpret_cast<LFSKNODE*>(temp & ADDR_MASK);
	}
	bool CAS(int level, LFSKNODE* old_ptr, LFSKNODE* new_ptr, 
		bool old_removed, bool new_removed) {
		long long old_value = reinterpret_cast<long long>(old_ptr);
		if (old_removed) old_value = old_value | 1;
		long long new_value = reinterpret_cast<long long>(new_ptr);
		if (new_removed) new_value = new_value | 1;
		return atomic_compare_exchange_strong(
			reinterpret_cast<volatile atomic_llong*>(&next[level]),
			&old_value,
			new_value);
	}
	~LFSKNODE() {}
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
	void Find(LFNODE*& prev, LFNODE*& curr, int key)
	{
	retry:
		prev = &head;
		curr = prev->GetNext();
		while (true)
		{
			bool removed;
			LFNODE* succ = curr->GetNext(&removed);
			while (removed)
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
			Find(prev, curr, key);

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
class C_SKSET {
	SKNODE head{ numeric_limits<int>::min(), TOP_LEVEL };
	SKNODE tail{ numeric_limits<int>::max(), TOP_LEVEL };
	mutex glock;
public:
	C_SKSET()
	{
		for (auto& n : head.next)
			n = &tail;
	}
	~C_SKSET() { Init(); }
	void Init()
	{
		SKNODE* ptr = head.next[0];
		while (ptr != &tail) {
			SKNODE* t = ptr;
			ptr = ptr->next[0];
			delete t;
		}
		for (auto& n : head.next)
			n = &tail;
	}
	void Find(int key, SKNODE* prev[], SKNODE* curr[])
	{
		for (int cl = TOP_LEVEL; cl >= 0; --cl) {
			if (cl == TOP_LEVEL)
				prev[cl] = &head;
			else
				prev[cl] = prev[cl + 1];
			curr[cl] = prev[cl]->next[cl];
			while (curr[cl]->key < key) {
				prev[cl] = curr[cl];
				curr[cl] = curr[cl]->next[cl];
			}
		}
	}
	bool Add(int key)
	{
		SKNODE* prev[TOP_LEVEL + 1], * curr[TOP_LEVEL + 1];
		glock.lock();
		Find(key, prev, curr);
		if (key == curr[0]->key) {
			glock.unlock();
			return false;
		}
		else {
			int nLevel = 0;
			for (nLevel = 0; nLevel < TOP_LEVEL; ++nLevel) 
				if (rand() % 2 == 1) break;
			
			SKNODE* node = new SKNODE(key, nLevel);
			for (int i = 0; i <= nLevel; ++i)
			{
				node->next[i] = curr[i];
				prev[i]->next[i] = node;
			}

			glock.unlock();
			return true;
		}
	}
	bool Remove(int key)
	{
		SKNODE* prev[TOP_LEVEL + 1], * curr[TOP_LEVEL + 1];
		glock.lock();
		Find(key, prev, curr);
		if (key == curr[0]->key) {
			for (int i = 0; i <= curr[0]->top_level; ++i)
				prev[i]->next[i] = curr[0]->next[i];
			glock.unlock();
			delete curr[0];
			return true;
		}
		else {
			glock.unlock();
			return false;
		}
	}
	bool Contains(int key)
	{
		SKNODE* prev[TOP_LEVEL + 1], * curr[TOP_LEVEL + 1];
		glock.lock();
		Find(key, prev, curr);
		if (key == curr[0]->key) {
			glock.unlock();
			return true;
		}
		else {
			glock.unlock();
			return false;
		}
		return false;
	}
	void PrintTwenty() {
		SKNODE*p = head.next[0];
		for (int i = 0; i < 20; ++i) {
			if (p == &tail) break;
			cout << p->key << ", ";
			p = p->next[0];
		}
		cout << endl;
	}
};
class L_SKSET {
	SKNODE head{ numeric_limits<int>::min(), TOP_LEVEL };
	SKNODE tail{ numeric_limits<int>::max(), TOP_LEVEL };
public:
	L_SKSET()
	{
		for (auto& n : head.next)
			n = &tail;

		head.bFullyLinked = tail.bFullyLinked = true;
	}
	~L_SKSET() { Init(); }
	void Init()
	{
		SKNODE* ptr = head.next[0];
		while (ptr != &tail) {
			SKNODE* t = ptr;
			ptr = ptr->next[0];
			delete t;
		}

		for (auto& n : head.next)
			n = &tail;
	}
	int Find(int key, SKNODE* prev[], SKNODE* curr[])
	{
		int nFindLevel = -1;
		for (int cl = TOP_LEVEL; cl >= 0; --cl) {
			if (cl == TOP_LEVEL)
				prev[cl] = &head;
			else
				prev[cl] = prev[cl + 1];
			curr[cl] = prev[cl]->next[cl];
			while (curr[cl]->key < key) {
				prev[cl] = curr[cl];
				curr[cl] = curr[cl]->next[cl];
			}
			if ((curr[cl]->key == key) && (nFindLevel == -1))
				nFindLevel = cl;
		}
		return nFindLevel;
	}
	bool Add(int key)
	{
		int nLevel = 0;
		for (nLevel = 0; nLevel < TOP_LEVEL; ++nLevel)
			if (rand() % 2 == 1) break;

		SKNODE* node = new SKNODE(key, nLevel);

		SKNODE* prev[TOP_LEVEL + 1], * curr[TOP_LEVEL + 1];
		while (1)
		{
			Find(key, prev, curr);
			if (key == curr[0]->key) {
				if (!curr[0]->bRemoved) {
					while (!curr[0]->bFullyLinked) {}
					return false;
				}
				continue;
			}

			bool invalid = false;
			int nLockedTop = 0;
			for (int i = 0; i <= nLevel; ++i) {
				prev[i]->nlock.lock();
				if ((prev[i]->bRemoved == true) ||
					(curr[i]->bRemoved == true) ||
					(prev[i]->next[i] != curr[i])) {
					nLockedTop = i;
					invalid = true;
					break;
				}
			}

			if (invalid) {
				for (int i = 0; i <= nLockedTop; ++i)
					prev[i]->nlock.unlock();
				continue;
			}

			for (int i = 0; i <= nLevel; ++i)
			{
				node->next[i] = curr[i];
				prev[i]->next[i] = node;
			}

			node->bFullyLinked = true;

			for (int i = 0; i <= nLevel; ++i)
				prev[i]->nlock.unlock();
			return true;
		}
	}
	bool Remove(int key)
	{	
		SKNODE* prev[TOP_LEVEL + 1], * curr[TOP_LEVEL + 1];
		
		int nLevel = Find(key, prev, curr);
		if (nLevel == -1) return false;

		SKNODE* victim = curr[nLevel];
		if (victim->bFullyLinked == false) return false;
		if (victim->bRemoved == true) return false;
		if (victim->top_level != nLevel) return false;

		victim->nlock.lock();
		if (victim->bRemoved) {
			victim->nlock.unlock();
			return false;
		}
		
		victim->bRemoved = true;
		
		int top_level = victim->top_level;

		while (true) {
			bool invalid = false;
			int nLockedTop = 0;
			for (int i = 0; i <= top_level; ++i) {
				prev[i]->nlock.lock();
				if ((prev[i]->bRemoved == true) || (prev[i]->next[i] != victim)) {
					invalid = true;
					nLockedTop = i;
					break;
				}
			}
			if (invalid) {
				for (int i = 0; i <= nLockedTop; ++i)
					prev[i]->nlock.unlock();
				nLevel = Find(key, prev, curr);
				continue;
			}
			for (int i = nLevel; i >= 0; --i)
				prev[i]->next[i] = victim->next[i];
			for (int i = nLevel; i >= 0; --i)
				prev[i]->nlock.unlock();
			victim->nlock.unlock();
			return true;
		}
	}
	bool Contains(int key)
	{
		SKNODE* prev[TOP_LEVEL + 1], * curr[TOP_LEVEL + 1];
		int nLevel = Find(key, prev, curr);
		if (nLevel == -1) return false;

		SKNODE* target = curr[nLevel];
		return (true == target->bFullyLinked &&
			false == target->bRemoved);
	}
	void PrintTwenty() {
		SKNODE* p = head.next[0];
		for (int i = 0; i < 20; ++i) {
			if (p == &tail) break;
			cout << p->key << ", ";
			p = p->next[0];
		}
		cout << endl;
	}
};
class LF_SKSET {
	LFSKNODE head{ numeric_limits<int>::min(), TOP_LEVEL };
	LFSKNODE tail{ numeric_limits<int>::max(), TOP_LEVEL };
public:
	LF_SKSET()
	{
		for (int i = 0; i <= TOP_LEVEL; ++i) {
			head.Set(i, &tail, false);
		}
	}
	~LF_SKSET() { Init(); }
	void Init()
	{
		LFSKNODE* ptr = head.Get(0);
		while (ptr != &tail) {
			LFSKNODE* t = ptr;
			ptr = ptr->Get(0);
			delete t;
		}

		for (int i = 0; i <= TOP_LEVEL; ++i) {
			head.Set(i, &tail, false);
		}
	}
	bool Find(int key, LFSKNODE* prev[], LFSKNODE* curr[])
	{
		retry:
		prev[TOP_LEVEL] = &head;
		for (int cl = TOP_LEVEL; cl >= 0; --cl) {
			if (cl != TOP_LEVEL)
				prev[cl] = prev[cl + 1];

			while (1) {
				curr[cl] = prev[cl]->Get(cl);

				bool removed = false;
				LFSKNODE* succ = curr[cl]->Get(cl, &removed);

				while (removed) {
					if (false == prev[cl]->CAS(cl, curr[cl], succ, false, false))
						goto retry;
					curr[cl] = succ;
					succ = curr[cl]->Get(cl, &removed);
				}

				if (curr[cl]->key >= key) break;
			}
		}
		return curr[0]->key == key;
	}
	bool Add(int key)
	{
		int nLevel = 0;
		for (nLevel = 0; nLevel < TOP_LEVEL; ++nLevel)
			if (rand() % 2 == 1) break;

		SKNODE* node = new SKNODE(key, nLevel);

		SKNODE* prev[TOP_LEVEL + 1], * curr[TOP_LEVEL + 1];
		while (1)
		{
			Find(key, prev, curr);
			if (key == curr[0]->key) {
				if (!curr[0]->bRemoved) {
					while (!curr[0]->bFullyLinked) {}
					return false;
				}
				continue;
			}

			bool invalid = false;
			int nLockedTop = 0;
			for (int i = 0; i <= nLevel; ++i) {
				prev[i]->nlock.lock();
				if ((prev[i]->bRemoved == true) ||
					(curr[i]->bRemoved == true) ||
					(prev[i]->next[i] != curr[i])) {
					nLockedTop = i;
					invalid = true;
					break;
				}
			}

			if (invalid) {
				for (int i = 0; i <= nLockedTop; ++i)
					prev[i]->nlock.unlock();
				continue;
			}

			for (int i = 0; i <= nLevel; ++i)
			{
				node->next[i] = curr[i];
				prev[i]->next[i] = node;
			}

			node->bFullyLinked = true;

			for (int i = 0; i <= nLevel; ++i)
				prev[i]->nlock.unlock();
			return true;
		}
	}
	bool Remove(int key)
	{
		LFSKNODE* prev[TOP_LEVEL + 1], * curr[TOP_LEVEL + 1];

		if(false == Find(key, prev, curr)) return false;

		int top_level = curr[0]->top_level;
		LFSKNODE* r_node = curr[0];
		for (int i = TOP_LEVEL; i > 0; --i) {
			bool removed = false;
			LFSKNODE* succ = r_node->Get(i, &removed);
			while (false == removed) {
				//�����ϴ���
				if (removed) break;

				bool ret = r_node->CAS(i, succ, succ, false, true);
				if (true == ret) break;
			}
		}

		bool removed = false;
		LFSKNODE* succ = r_node->Get(0, &removed);

		while (1) {
			bool ret = r_node->CAS(0, succ, succ, false, true);
			succ = r_node->Get(0, &removed);
			if (ret) {
				Find(key, prev, curr);
				return true;
			}
			else if (removed) return false;
		}
	}
	bool Contains(int key)
	{
		SKNODE* prev[TOP_LEVEL + 1], * curr[TOP_LEVEL + 1];
		int nLevel = Find(key, prev, curr);
		if (nLevel == -1) return false;

		SKNODE* target = curr[nLevel];
		return (true == target->bFullyLinked &&
			false == target->bRemoved);
	}
	void PrintTwenty() {
		LFSKNODE* p = head.Get(0);
		for (int i = 0; i < 20; ++i) {
			if (p == &tail) break;
			cout << p->key << ", ";
			p = p->Get(0);
		}
		cout << endl;
	}
};

typedef L_SKSET MY_SET;

MY_SET mySet;

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