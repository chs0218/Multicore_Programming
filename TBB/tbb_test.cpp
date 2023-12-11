/*
���� - Nuget ��Ű�� ������ - �ַ��ǿ� Nuget ��Ű�� ���� - intelltbb ��ġ

concurrent_unordered_map
- insert(), find(), count(), size(), at()�� thread_safe�ϴ�.
- erase()�� thread_safe���� �ʴ�.
erase�� �־��� ��, ������ ���� �ʱ� ������..
���� �����͸� �־��� ���� �ϴ� �뵵�δ� ����� �� ����.

// 11�� ����, 15�� �⸻����

tbb�� �ִ� �� �츮�� ����� �ͺ��� ��κ� ����.
�׷��� ���� thread_safe ���� ���� �޼ҵ尡 �ֱ� ������ �� �˾ƺ��� ����Ѵ�.
STL ������ �̹��ϰ� �ٸ���.

ũ�� ���� ���� ������ vector�� thread_safe���� �ʴ�.
vector�� resize ������ �迭�̴�.
vector�� �޸� ���� ������ ��Ƽ�����忡�� ����� �� ����.

concurrent_vector�� ��� shrink�� �Ұ����ϴ�.
push_back(), grow_by() ���� �޼ҵ带 �����Ѵ�.
clear(), swap(), resize(), reserve()�� ���ļ����� �� ����.
���ҵ��� ���ӵ� �ּҿ� ���� �ʴ�.
�߰��� ��ֹ��� ������ �Ѿ allocation �Ѵ�.
���Ҹ� ���� ��, Data Dependency ������ �ؾ��Ѵ�.
���� ��, ���� ���� �� �ִ�.

concurrent_queue�� ���
try_pop(), push()�޼ҵ带 �����Ѵ�.
empty() ȣ���� pop�� ������ �������� �ʱ� ������ try_pop() �޼ҵ带 �����Ѵ�.
unsafe_size(), empty(), clear(), swap()�� thread_safe���� �ʴ�.

RwLock
ReadOnly �޼ҵ�� ��ȣ���� ���� �����Ѵ�.
Write �޼ҵ���� Write Lock�� �ɰ� Read �޼ҵ���� Read Lock�� �Ǵ�.
- search, size, empty �� << Read �޼ҵ�
- Read Lock ������ ���ÿ� ����ǰ� �Ѵ�.

�׳� Locking���ٴ� ��������.
Set�̳� Map ���� Read�� ������ ���� �����̳ʿ��� ����ϱ⵵ �Ѵ�.
std::shared_lock

Scalable Lock: busy waiting�� ���� CPU ���� ���´�. �׷��� �ü�� ȣ��� ���� overhaed�� ũ��.
Fair Lock: �Ӱ迵���� ������ ������� lock�� ��´�.
Recursive: ���� ������� lock�� �������� ���� �� �ִ�.
Long Wait�� ��� yield Ȥ�� block

CUDA
- ����ó���� CPU�� �ƴ� GPU���� �����Ѵ�.
- GPU�� CPU���� ���� ������ ������!!
- �������� DirectX 12�� �̿��� GPU�� �ؽ��ĳ� ������ ���·� ������(��ġ, ...)�� ������
��� ����� �� ���� ������ ���ۿ� ���� �����͸� �ٽ� CPU�� �о�� ���
- ������ DirectX 12, HLSL ���� �˾ƾ��Ѵ�..? �����ϴ�.
- �̸� �����ϰ� �Ȱ� NVIDIA���� ��ǥ�� CUDA
- ������ NVIDIA �ϵ������� ����
- IO �ӵ��� ��� ������ ���� ��� �ӵ��� ������.
- ���� �������� ����� ���ؼ��� CUDA�� ���⵵ �Ѵ�.
- �ǽð� �����Ϳ��� ���� ��ƴ�.
- �޸𸮰� ����. (24G..?)

- ����� �ǽð� �ùķ������� CUDA�� �غ����� �׳� CPU���� �ϴ°� �� �����ٶ�� ����� ���Ծ���. 

Ʈ�����
- ��Ƽ������ ���α׷��� �������� �����ϱ� ���� ���ȵ� ���α׷��� ��
- �Ӱ� ������ Ʈ��������� ����, �� Ʈ������� Atomic�ϰ� ����
- �ٸ� ������� ���� ���� �ϴ� ������ �ϰ� �浹 �˻縦 ����
- �浹�� ������ Commit(���� Ȯ�� �� Fix), �浹�� ������ Abort(��� ������ ��ȿȭ)
- ����ȭ �浹 ���� Ʈ������� ��� ����Ǵ� ���ѷ���, ���� �߻� ���� ��Ȳ�� ���� �� ������
���α׷��Ӱ� �Ű� ������Ѵ�.
- ������尡 ũ��(�̱� ������ ���α׷����� ������ �� ������. ���, ���� �ھ ��������)

Ʈ������� ����
- �ϵ���� Ʈ������� ��� Cache �ϰ��� ���������� �����ؼ� ����
- Cache�� �ϴ� ���� ���� �ϰ����� �˻�, �����Ұ�� �޸𸮿� ���� �ʰ� ����Ѵ�.
- CPU�� ���൵ abort�� �ƹ��͵� ������� ��ó�� �ٲ۴�.
- �������� �޸𸮿� ���� Transaction�� ���(����, ������ ����. L1 ĳ���� �뷮 �ѵ�����)
- CPU������ transaction ���н��� ������ ����. �޸𸮿� ���������� ������ ��� Roll-back

���ο� ���?
- C ��Ÿ�� �� ����ϱ� ������ ���� ��� �Ѱ谡 �ִ�.
- ������ ������ �޸� ����, side effect(�Ķ���Ͱ� �ƴ� ���� ���� ����) ������
- �Լ��� �� ����غ���.
- �޸� ������ ���� �ʰ� side effect�� ����.
- ���α׷� ��ü�� �ڿ������� ���ļ��� ����Ǿ� �ִ�.
- ��� ������ const�̴�.
- �Լ��� ȣ���ϴ� ������� ���α׷����Ѵ�.

Go
- ���ۿ��� ���� C�� ����� ���
- ���� ��Ƽ�����带 ����
- Goroutine���� �׻� ������ ������ ������ �����ϸ鼭 task�� �Ҵ��Ѵ�.
- data race ����, mutex�� �ʿ��ϸ� ���!
- �׷� Intel TBB�� �ٸ��� ���� �ʴ���..?
- ��Ƽ������ ������ ���� ��å���� Goroutine ������ ���� ����� ������ Channel�� �����Ѵ�.
- Lock-free Queue�� �����Ǿ��ִ�.

�Ͻ���
- ���� �Լ��� ���
- 1990�⿡ ����
- ������ �پ���� ���̵��� ���� ���� ������ ���ϰ� �ִ�.

Erlang
- �Լ��� ���, Process ������ ���ļ�
- �ü�� Process�� �ƴ� Coroutine�� ������.
- ���� �޸� ���� �޼��� �н����� ����

Elixir
- Erlang�� ����� �� �Լ��� ���� ���
- Send�� Receive�� ����� ����ȭ

��Ƽ������ ���α׷����� ���� �� ����.
�Ϲ� ���α׷��ֿ����� �� �� ���� ���� ��ġ ���� ������ �־
�ǵ��� ��Ƽ������ ���α׷����� ���ؾ������� ���� �� ����.
�������� ������� ������ �ذ��� �� ������ ������ �����ؾ��Ѵ�.
���� �Ը� �̻��� ��Ƽ������ ���α׷������� ��Ƽ������� �ڷᱸ���� �ʿ��ϴ�.
�׸� ���ؼ� Non-Blocking ��Ƽ������ �˰������� ����� �ʿ��ϴ�.
�ڽ��� �䱸�� �� �´� Custom�� ���� �˰������� ���� �ۼ��ؼ� ����ϴ� ���� ���� ������ ������ ��ƴ�.
Transactional Memory, Functional Programming�� �����غ���.

*/

#include <tbb/parallel_for.h>
#include <iostream>
#include <chrono>
#include <vector>
#include <thread>

using namespace std;
using namespace tbb;
using namespace std::chrono;

int main()
{
	{
		atomic_int sum = 0;
		auto start_t = system_clock::now();
		for (int i = 0; i < 5000'0000; ++i)
			sum += 2;
		auto end_t = system_clock::now();
		auto exec_t = end_t - start_t;
		auto ms = duration_cast<milliseconds>(exec_t).count();

		cout << "Single Thread Sum = " << sum << "\n";
		cout << "Exec: " << ms << "ms\n";
	}
	{
		const int num_threads = thread::hardware_concurrency();
		atomic_int sum = 0;

		vector<thread> threads;
		auto start_t = system_clock::now();

		for (int i = 0; i < num_threads; ++i)
		{
			threads.emplace_back(thread{ [&sum, num_threads]() {
				for (int i = 0; i < 5000'0000 / num_threads; ++i)
					sum += 2;
				} });
		}

		for (auto& th : threads) th.join();
		auto end_t = system_clock::now();
		auto exec_t = end_t - start_t;
		auto ms = duration_cast<milliseconds>(exec_t).count();

		cout << "Multi Thread Sum = " << sum << "\n";
		cout << "Exec: " << ms << "ms\n";
	}
	{
		atomic_int sum = 0;
		auto start_t = system_clock::now();

		parallel_for(0, 5000'0000, [&](int i) { sum += 2; });

		auto end_t = system_clock::now();
		auto exec_t = end_t - start_t;
		auto ms = duration_cast<milliseconds>(exec_t).count();

		cout << "TBB Sum = " << sum << "\n";
		cout << "Exec: " << ms << "ms\n";
	}
}