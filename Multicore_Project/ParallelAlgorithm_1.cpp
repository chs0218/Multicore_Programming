#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include <limits>

using namespace std;
using namespace std::chrono;

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