#include <fstream>

using namespace std;

class Graph {
public:
	Graph(char* path);
	~Graph();
	void createGraphFromFile();		// ���ļ��ж�ȡͼ
	void printGraph();			// ��ӡλͼ

	bool* getGraph();
	int getNodesNum();


private:
	char* path;				// �ļ�·��
	bool* bitMap;			// ʹ��λͼ�洢�ڽӱ�
	long long int numNodes;			// �����
};