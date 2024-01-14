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

int* hybridBFS(bool* graph, int numNodes, int root);		// CPU��ϵ���BFS
int* serialBFS(bool* graph, int numNodes, int root);			// ����
int* queueBased(bool* graph, int numNodes, int root);		// ���ڶ���
int* readBased(bool* graph, int numNodes, int root);		// ���ڶ�
vector<int> findNeighNodes(bool* graph, int numNodes, int curr);	// Ѱ��ĳ�������ھӽ��