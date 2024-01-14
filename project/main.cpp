#define _CRT_SECURE_NO_WARNINGS
//#include <cuda_runtime.h>
//#include <device_launch_parameters.h>

#include <omp.h> 
#include "graph.h"
#include "cpuBFS.h"
//#include "gpu.cuh"
#define TEST_TIMES 10

using namespace std;
using namespace std::chrono;


int concurentThreadsSupported;

void showResult(int* lev, int nodes) {
	int maxLev = -1;
	for (int i = 0; i < nodes; i++) {
		if (lev[i] > maxLev) {
			maxLev = lev[i];
		}
	}

	int* count = new int[maxLev + 2];
	for (int i = 0; i < maxLev + 2; i++) {
		count[i] = 0;
	}
	for (int i = 0; i < nodes; i++) {
		count[lev[i] + 1]++;
	}


	for (int i = 1; i < maxLev + 2; i++) {
		printf("�㼶 %d �Ľ����Ϊ %d��ռ��%.1f %%\n", i - 1, count[i], (float)((count[i]) * 100) / nodes);
	}

	delete[] count;
}


int main(int argc, char* argv[]) {
	omp_set_num_threads(omp_get_num_procs());
	concurentThreadsSupported = omp_get_num_procs();
	printf("֧���߳�: %d\n", concurentThreadsSupported);

	//printf("hello from CPU\n");
	//hello();

	char choice;
	printf("ѡ��һ���㷨:\n");
	printf("S: ����BFS\n");
	printf("Q: ����Queue��BFS\n");
	printf("R: ����Read��BFS\n");
	printf("H: CPU��ϵ���BFS\n");

	scanf("%c", &choice);

	// For time computation
	time_point<steady_clock> start;
	time_point<steady_clock> stop;
	milliseconds* duration = new milliseconds[10];


	// Import graph
	char path[50];
	const int root = 0;
	int* lev;
	int numNodes = -1;
	bool* graph;

	// ��ȡͼ
	printf("\n������ͼ�ļ����ƣ�");
	scanf("%s", &path);
	printf("%s\n", path);

	printf("\n��������ͼ����ȴ�...\n");
	Graph G(path);
	graph = G.getGraph();
	numNodes = G.getNodesNum();
	printf("%d\n", numNodes);
	printf("ͼ�������\n");


	T2 = (int)max((double)T2, (double)numNodes * 0.01);

	// ��������ִ��BFS
	for (int i = 0; i < TEST_TIMES; i++) {

		printf("ִ�д���: %d\n", i+1, TEST_TIMES);
		start = high_resolution_clock::now();
		switch (choice) {
		case 'H':
			lev = hybridBFS(graph, numNodes, root);
			break;
		case 'R':
			lev = readBased(graph, numNodes, root);
			break;
		case 'Q':
			lev = queueBased(graph, numNodes, root);
			break;
		case 'S':
			lev = serialBFS(graph, numNodes, root);
			break;
		default:
			lev = NULL;
			break;
		}
		stop = chrono::high_resolution_clock::now();
		duration[i] = duration_cast<milliseconds>(stop - start);
		printf("����ʱ��: %d ms\n", (int)duration[i].count());

		if (i == TEST_TIMES - 1) {
			printf("\n");
			showResult(lev, numNodes);
		}

		delete[] lev;
	}

	int sum = 0;
	for (int i = 0; i < TEST_TIMES; i++) {
		sum += (int)duration[i].count();
		printf("�� %d �Σ�����ʱ�䣺%d ms\n", i, (int)duration[i].count());
	}
	printf("ƽ������ʱ�䣺%d\n", sum / TEST_TIMES);

	return 0;
}