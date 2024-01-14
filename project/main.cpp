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
		printf("层级 %d 的结点数为 %d，占比%.1f %%\n", i - 1, count[i], (float)((count[i]) * 100) / nodes);
	}

	delete[] count;
}


int main(int argc, char* argv[]) {
	omp_set_num_threads(omp_get_num_procs());
	concurentThreadsSupported = omp_get_num_procs();
	printf("支持线程: %d\n", concurentThreadsSupported);

	//printf("hello from CPU\n");
	//hello();

	char choice;
	printf("选择一个算法:\n");
	printf("S: 串行BFS\n");
	printf("Q: 基于Queue的BFS\n");
	printf("R: 基于Read的BFS\n");
	printf("H: CPU混合调度BFS\n");

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

	// 读取图
	printf("\n请输入图文件名称：");
	scanf("%s", &path);
	printf("%s\n", path);

	printf("\n正在载入图，请等待...\n");
	Graph G(path);
	graph = G.getGraph();
	numNodes = G.getNodesNum();
	printf("%d\n", numNodes);
	printf("图载入完成\n");


	T2 = (int)max((double)T2, (double)numNodes * 0.01);

	// 根据命令执行BFS
	for (int i = 0; i < TEST_TIMES; i++) {

		printf("执行次数: %d\n", i+1, TEST_TIMES);
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
		printf("运行时间: %d ms\n", (int)duration[i].count());

		if (i == TEST_TIMES - 1) {
			printf("\n");
			showResult(lev, numNodes);
		}

		delete[] lev;
	}

	int sum = 0;
	for (int i = 0; i < TEST_TIMES; i++) {
		sum += (int)duration[i].count();
		printf("第 %d 次，运行时间：%d ms\n", i, (int)duration[i].count());
	}
	printf("平均运行时间：%d\n", sum / TEST_TIMES);

	return 0;
}