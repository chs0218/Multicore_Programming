/*
도구 - Nuget 패키지 관리자 - 솔류션용 Nuget 패키지 관리 - intelltbb 설치

concurrent_unordered_map
- insert(), find(), count(), size(), at()이 thread_safe하다.
- erase()가 thread_safe하지 않다.
erase를 넣었을 때, 성능이 좋지 않기 때문에..
따라서 데이터를 넣었다 뺐다 하는 용도로는 사용할 수 없다.

// 11일 종강, 15일 기말고사

tbb에 있는 건 우리가 만드는 것보다 대부분 좋다.
그러나 가끔 thread_safe 하지 않은 메소드가 있기 때문에 잘 알아보고 써야한다.
STL 사용법과 미묘하게 다르다.

크기 증가 감소 때문에 vector는 thread_safe하지 않다.
vector는 resize 가능한 배열이다.
vector의 메모리 접근 때문에 멀티쓰레드에서 사용할 수 없다.

concurrent_vector의 경우 shrink는 불가능하다.
push_back(), grow_by() 등의 메소드를 제공한다.
clear(), swap(), resize(), reserve()는 병렬수행할 수 없다.
원소들이 연속된 주소에 있지 않다.
중간에 장애물이 있으면 넘어서 allocation 한다.
원소를 읽을 때, Data Dependency 고려를 해야한다.
읽을 때, 생성 중일 수 있다.

concurrent_queue의 경우
try_pop(), push()메소드를 제공한다.
empty() 호출이 pop의 성공을 보장하지 않기 때문에 try_pop() 메소드를 제공한다.
unsafe_size(), empty(), clear(), swap()은 thread_safe하지 않다.

RwLock
ReadOnly 메소드는 상호배제 없이 실행한다.
Write 메소드들은 Write Lock을 걸고 Read 메소드들은 Read Lock을 건다.
- search, size, empty 등 << Read 메소드
- Read Lock 끼리는 동시에 실행되게 한다.

그냥 Locking보다는 빨라진다.
Set이나 Map 같은 Read의 비중이 높은 컨테이너에서 사용하기도 한다.
std::shared_lock

Scalable Lock: busy waiting을 없애 CPU 낭비를 막는다. 그러나 운영체제 호출로 인해 overhaed가 크다.
Fair Lock: 임계영역에 도착한 순서대로 lock을 얻는다.
Recursive: 같은 쓰레드는 lock을 다중으로 얻을 수 있다.
Long Wait의 경우 yield 혹은 block

CUDA
- 병렬처리를 CPU가 아닌 GPU에서 수행한다.
- GPU가 CPU보다 몇백배 빠르기 때문에!!
- 예전에는 DirectX 12를 이용해 GPU에 텍스쳐나 폴리곤 형태로 데이터(위치, ...)를 전달해
행렬 계산을 한 다음 프레임 버퍼에 적힌 데이터를 다시 CPU에 읽어와 사용
- 하지만 DirectX 12, HLSL 등을 알아야한다..? 불편하다.
- 이를 지원하게 된게 NVIDIA에서 발표한 CUDA
- 하지만 NVIDIA 하드웨어에서만 지원
- IO 속도가 배우 느리고 직렬 계산 속도가 느리다.
- 게임 물리엔진 계산을 위해서는 CUDA를 쓰기도 한다.
- 실시간 데이터에는 쓰기 어렵다.
- 메모리가 적다. (24G..?)

- 듀랑고 실시간 시뮬레이팅은 CUDA로 해봤지만 그냥 CPU에서 하는게 더 빠르다라는 결론이 나왔었다. 

트랜잭션
- 멀티쓰레드 프로그래밍 단점들을 보완하기 위해 고안된 프로그래밍 모델
- 임계 영역을 트랜잭션으로 정의, 각 트랜잭션은 Atomic하게 실행
- 다른 쓰레드와 관계 없이 일단 실행을 하고 충돌 검사를 진행
- 충돌이 없으면 Commit(실행 확정 후 Fix), 충돌이 있으면 Abort(모든 실행을 무효화)
- 동기화 충돌 이후 트랜잭션이 계속 실행되다 무한루프, 예외 발생 등의 상황이 생길 수 있으며
프로그래머가 신경 써줘야한다.
- 오버헤드가 크다(싱글 쓰레드 프로그램보다 성능이 더 느리다. 듀얼, 쿼드 코어도 마찬가지)

트랜잭션의 구현
- 하드웨어 트랜잭션의 경우 Cache 일관성 프로토콜을 수정해서 구현
- Cache에 일단 값을 쓰고 일관성을 검사, 실패할경우 메모리에 쓰지 않고 폐기한다.
- CPU의 실행도 abort해 아무것도 실행안한 것처럼 바꾼다.
- 여러개의 메모리에 대해 Transaction을 허용(물론, 제한은 있음. L1 캐시의 용량 한도까지)
- CPU에서는 transaction 실패시의 복구를 제공. 메모리와 레지스터의 변경을 모두 Roll-back

새로운 언어?
- C 스타일 언어를 사용하기 때문에 성능 향상에 한계가 있다.
- 쓰레드 사이의 메모리 공유, side effect(파라미터가 아닌 전역 변수 수정) 때문에
- 함수형 언어를 사용해보자.
- 메모리 공유를 하지 않고 side effect가 없다.
- 프로그램 자체에 자연스럽게 병렬성이 내장되어 있다.
- 모든 변수가 const이다.
- 함수를 호출하는 방식으로 프로그래밍한다.

Go
- 구글에서 만든 C와 비슷한 언어
- 언어에서 멀티쓰레드를 지원
- Goroutine으로 항상 최적의 쓰레드 개수를 유지하면서 task를 할당한다.
- data race 존재, mutex를 필요하면 써라!
- 그럼 Intel TBB랑 다른게 없지 않느냐..?
- 멀티쓰레드 문제에 대한 대책으로 Goroutine 사이의 고속 통신이 가능한 Channel를 지원한다.
- Lock-free Queue로 구현되어있다.

하스켈
- 순수 함수형 언어
- 1990년에 개발
- 개념은 뛰어나지만 난이도로 인해 많이 사용되지 못하고 있다.

Erlang
- 함수형 언어, Process 단위의 병렬성
- 운영체제 Process가 아닌 Coroutine에 가깝다.
- 공유 메모리 없이 메세지 패싱으로 구현

Elixir
- Erlang에 기반을 둔 함수형 병렬 언어
- Send와 Receive를 사용해 동기화

멀티쓰레드 프로그래밍은 피할 수 없다.
일반 프로그래밍에서는 볼 수 없는 많은 골치 아픈 문제가 있어서
되도록 멀티쓰레드 프로그래밍을 피해야하지만 피할 수 없다.
여러가지 방법으로 문제를 해결할 수 있지만 성능을 고려해야한다.
일정 규모 이상의 멀티쓰레드 프로그램에서는 멀티쓰레드용 자료구조가 필요하다.
그를 위해선 Non-Blocking 멀티쓰레드 알고리즘의 사용이 필요하다.
자신의 요구에 딱 맞는 Custom한 병렬 알고리즘을 직접 작성해서 사용하는 것이 제일 성능이 좋지만 어렵다.
Transactional Memory, Functional Programming도 고려해보자.

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