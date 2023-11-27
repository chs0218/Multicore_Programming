/*
멀티코어 프로그래밍 말고, 멀티코어를 사용하는 방법이 있나?
C++, OpenMP, Intel Thread Building Block, CUDA, Transactional Memory, 새로운 언어

RUST?

condition_variable, future, promise => 운영체제 호출
async => 미완성
coroutine => 멀티코어 구현이 안됨
게임이 아니라 작업이 커서 운영체제 호출이 단점이 아니라면 써도 된다.

OpenMP - 컴파일러 레벨에서 구현된다. 따라서 컴파일러가 지원하지 않으면 사용할 수 없다.
표준으로 지정되어 있어 대부분의 컴파일러에서 구현되어 있다.

90년대 초 SMP 컴퓨터에서 FORTRAN의 loop를 병렬수행하기 위해 개발
분산 메모리에서는 사용할 수 없다. 모든 코어에서 모든 메모리를 읽고 쓸 수 있어야한다.
네트워크로 연결되어있으면 사용할 수 없음

우리가 짠 프로그램을 최적화해주진 않는다. (최상의 공유 메모리 사용 패턴을 보장하지 않는다.)
Data Race, Deadlock, Data Dependency 검사는 프로그래머가 해야한다.
어느 부분을 어떻게 병렬화 할지를 프로그래머가 지정해주어야한다.

비주얼 스튜디오에서는 프로젝트 속성 - C/C++ - 언어 - OpenMP 지원을 예로 수정하면 된다.

Fork - Join 모델을 사용한다.
실행하다가 여러개의 코어를 한번에 돌려 작업을 병렬로 돌리고 병렬이 끝나면 한개의 코어로 돌아간다.
컴파일러 디렉티브에 의존한다.
Nesting 가능하다. 쓰레드 안에서 쓰레드를 만들 수 있다.

멀티 쓰레드가 생성되서 해당되는 블록의 코드를 병렬로 수행한다.
블록 끝에서 모든 쓰레드의 종료를 확인한 후 진행을 계속한다.
*/

#include <omp.h>
#include <stdio.h>
int main()
{
	int nthreads, tid;
	/* Fork a team of threads with each thread having a private tid variable */
#pragma omp parallel private(tid)
	{
		/* Obtain and print thread id */
		tid = omp_get_thread_num();
		printf("Hello World from thread = %d\n", tid);
		/* Only master thread does this */
		if (tid == 0) {
			nthreads = omp_get_num_threads();
			printf("Number of threads = %d\n", nthreads);
		}
	} /* All threads join master thread and terminate */
}
