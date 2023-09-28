#include <iostream>
#include <thread>

using namespace std;

int a;

void func() 
{
	for (int i = 0; i < 10; ++i)
	{
		cout << "i = " << i << endl;
		a = a + 1;
	}
}

int main()
{
	thread t1(func);
	thread t2(func);
	t1.join();
	t2.join();
	cout << "A = " << a;
}