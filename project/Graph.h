#include <fstream>

using namespace std;

class Graph {
public:
	Graph(char* path);
	~Graph();
	void createGraphFromFile();		// 从文件中读取图
	void printGraph();			// 打印位图

	bool* getGraph();
	int getNodesNum();


private:
	char* path;				// 文件路径
	bool* bitMap;			// 使用位图存储邻接表
	long long int numNodes;			// 结点数
};