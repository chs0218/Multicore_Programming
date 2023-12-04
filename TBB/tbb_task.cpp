#include <tbb/task_group.h>
#include <iostream>
#include <chrono>
#include <vector>
#include <thread>

int s_fibo(int n)
{
	if (n < 2)
		return n;
	else
		return s_fibo(n - 1) + s_fibo(n - 2);
}

void mt_fibo(int n, int& res)
{
	if (n < 2)
		res = n;
	else {
		int x, y;
		std::thread t1{ mt_fibo, n - 1, std::ref(x) };
		std::thread t2{ mt_fibo, n - 2, std::ref(y) };

		t1.join();
		t2.join();

		res = x + y;
	}
}

int tbb_fibo(int n)
{
	if (n < 30)
		return s_fibo(n);
	else {
		tbb::task_group tg;
		int x, y;
		tg.run([&x, n]() { x = tbb_fibo(n - 1); });
		tg.run([&y, n]() { y = tbb_fibo(n - 2); });
		tg.wait();
		return x + y;
	}
}

int main()
{
	{
		std::atomic_int sum = 0;
		auto start_t = std::chrono::system_clock::now();
		sum = s_fibo(45);
		auto end_t = std::chrono::system_clock::now();
		auto exec_t = end_t - start_t;
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(exec_t).count();

		std::cout << "Single Thread Sum = " << sum << "\n";
		std::cout << "Exec: " << ms << "ms\n";
	}
	/*{
		auto start_t = std::chrono::system_clock::now();
		int sum = 0;
		mt_fibo(45, sum);
		auto end_t = std::chrono::system_clock::now();
		auto exec_t = end_t - start_t;
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(exec_t).count();

		std::cout << "Multi Thread Sum = " << sum << "\n";
		std::cout << "Exec: " << ms << "ms\n";
	}*/
	{
		auto start_t = std::chrono::system_clock::now();
		int sum = tbb_fibo(45);
		auto end_t = std::chrono::system_clock::now();
		auto exec_t = end_t - start_t;
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(exec_t).count();

		std::cout << "Multi Thread Sum = " << sum << "\n";
		std::cout << "Exec: " << ms << "ms\n";
	}
}