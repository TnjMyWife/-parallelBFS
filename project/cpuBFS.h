#include <mutex>
#include <omp.h> 
#include <vector>
#include <queue>
using namespace std;
#define SEQ 0
#define QUEUE 1
#define QUEUE_TO_READ 2
#define READ 3
#define READ_TO_QUEUE 4



extern int T1;
extern int T2;
extern int T3;

int* hybridBFS(bool* graph, int numNodes, int root);		// CPU混合调度BFS
int* serialBFS(bool* graph, int numNodes, int root);			// 串行
int* queueBased(bool* graph, int numNodes, int root);		// 基于队列
int* readBased(bool* graph, int numNodes, int root);		// 基于读
vector<int> findNeighNodes(bool* graph, int numNodes, int curr);	// 寻找某个结点的邻居结点