#define _CRT_SECURE_NO_WARNINGS
# include "Graph.h"
// 将文件中的整数字符转化成整数
int charsToInt(char chars[], int length) {
	bool flag = true;
	int dim = 0;
	for (int i = 0; i < length && flag; i++) {
		if (chars[i] == '\0') {
			dim = i;
			flag = false;
		}
	}
	int output = 0;
	for (int i = dim - 1, digit = 1; i >= 0; i--, digit *= 10) {
		if (chars[i] == '-') {
			return -1 * output;
		}
		output += (chars[i] - '0') * digit;
	}
	return output;
}

Graph::Graph(char* graph_path):
	path(graph_path)
{
	createGraphFromFile();
}

bool* Graph::getGraph() { return bitMap; }
int Graph::getNodesNum() { return numNodes; }

// 从文件中读取图结构
void Graph::createGraphFromFile()
{
	ifstream inFile;
	inFile.open(path);
	if (!inFile) {
		printf("文件不存在"); 
		exit(1);
	}

	char curr[256];
	if (sscanf(path, "%lldk.txt", &numNodes) == 1) {
	}
	else {
		printf("无法从文件名中提取结点数。\n");
	}
	numNodes *= 1000;
	long long int n = (long long int)numNodes;

	// 初始化邻接表
	bitMap = new bool[n * n];
	for (long long int i = 0; i < n * n; i++) {
		bitMap[i] = false;
	}
	printf("dididi\n");
	// 获取边并添加到领接表中
	long long int startId;
	long long int endId;
	while (!inFile.getline(curr, 256, '\t').eof()) {
		startId = charsToInt(curr, 256);

		inFile.getline(curr, 256, '\n');
		endId = charsToInt(curr, 256);

		bitMap[startId * n + endId] = true;
	}

	inFile.close();
}

void Graph::printGraph() {
	printf("\nGraph:\n");
	for (int i = 0; i < numNodes; i++) {
		for (int j = 0; j < numNodes; j++) {
			if (bitMap[i * numNodes + j]) {
				printf("1 ");
			}
			else {
				printf("0 ");
			}
		}
		printf("\n");
	}
	printf("\n");
}

Graph::~Graph()
{
	delete[] bitMap;
}