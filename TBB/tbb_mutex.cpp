#include <iostream>
#include <array>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <tbb/rw_mutex.h>
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
constexpr int NUM_TEST = 400'0000;
constexpr int KEY_RANGE = 1000;

class my_mutex
{
public:
	void lock() {}
	void unlock() {}
};
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

class RW_SET {
	NODE head, tail;
	tbb::rw_mutex glock;
public:
	RW_SET()
	{
		head.key = numeric_limits<int>::min();
		tail.key = numeric_limits<int>::max();
		head.next = &tail;
	}
	~RW_SET() { Init(); }
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
			delete curr;
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
		glock.lock_shared();
		curr = prev->next;
		while (curr->key < key) {
			prev = curr;
			curr = curr->next;
		}
		if (key == curr->key) {
			glock.unlock_shared();
			return true;
		}
		else {
			glock.unlock_shared();
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

RW_SET mySet;

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
		switch (rand() % 100) {
		case 0:
			key = rand() % KEY_RANGE;
			mySet.Add(key);
			break;
		case 1:
			key = rand() % KEY_RANGE;
			mySet.Remove(key);
			break;
		default:
			key = rand() % KEY_RANGE;
			mySet.Contains(key);
			break;
			/*default: cout << "Error\n";
				exit(-1);*/
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
	/*for (int i = 1; i <= MAX_THREADS; i *= 2)
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
	}*/

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