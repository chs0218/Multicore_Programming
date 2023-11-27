/*
��Ƽ�ھ� ���α׷��� ����, ��Ƽ�ھ ����ϴ� ����� �ֳ�?
C++, OpenMP, Intel Thread Building Block, CUDA, Transactional Memory, ���ο� ���

RUST?

condition_variable, future, promise => �ü�� ȣ��
async => �̿ϼ�
coroutine => ��Ƽ�ھ� ������ �ȵ�
������ �ƴ϶� �۾��� Ŀ�� �ü�� ȣ���� ������ �ƴ϶�� �ᵵ �ȴ�.

OpenMP - �����Ϸ� �������� �����ȴ�. ���� �����Ϸ��� �������� ������ ����� �� ����.
ǥ������ �����Ǿ� �־� ��κ��� �����Ϸ����� �����Ǿ� �ִ�.

90��� �� SMP ��ǻ�Ϳ��� FORTRAN�� loop�� ���ļ����ϱ� ���� ����
�л� �޸𸮿����� ����� �� ����. ��� �ھ�� ��� �޸𸮸� �а� �� �� �־���Ѵ�.
��Ʈ��ũ�� ����Ǿ������� ����� �� ����

�츮�� § ���α׷��� ����ȭ������ �ʴ´�. (�ֻ��� ���� �޸� ��� ������ �������� �ʴ´�.)
Data Race, Deadlock, Data Dependency �˻�� ���α׷��Ӱ� �ؾ��Ѵ�.
��� �κ��� ��� ����ȭ ������ ���α׷��Ӱ� �������־���Ѵ�.

���־� ��Ʃ��������� ������Ʈ �Ӽ� - C/C++ - ��� - OpenMP ������ ���� �����ϸ� �ȴ�.

Fork - Join ���� ����Ѵ�.
�����ϴٰ� �������� �ھ �ѹ��� ���� �۾��� ���ķ� ������ ������ ������ �Ѱ��� �ھ�� ���ư���.
�����Ϸ� ��Ƽ�꿡 �����Ѵ�.
Nesting �����ϴ�. ������ �ȿ��� �����带 ���� �� �ִ�.

��Ƽ �����尡 �����Ǽ� �ش�Ǵ� ����� �ڵ带 ���ķ� �����Ѵ�.
��� ������ ��� �������� ���Ḧ Ȯ���� �� ������ ����Ѵ�.
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
