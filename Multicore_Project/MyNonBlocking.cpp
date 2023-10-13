#include <iostream>
#include <array>
#include <vector>
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


				
*/


int main()
{

}