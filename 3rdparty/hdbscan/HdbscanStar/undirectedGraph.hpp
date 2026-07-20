#pragma once
#include<vector>
class undirectedGraph
{
private:
	int _numVertices;
	std::vector<int> _verticesA;
	std::vector<int> _verticesB;
	std::vector<double> _edgeWeights;
	std::vector<std::vector<int>> _edges;

public:
	undirectedGraph(int numVertices, std::vector<int> verticesA, std::vector<int> verticesB, std::vector<double> edgeWeights)
	{
		_numVertices = numVertices;
		_verticesA = verticesA;
		_verticesB = verticesB;
		_edgeWeights = edgeWeights;
		_edges.resize(numVertices);
		int _edgesLength = _edges.size();
		int _edgeWeightsLength = _edgeWeights.size();
		for (int i = 0; i < _edgeWeightsLength; i++)
		{
			_edges[_verticesA[i]].push_back(_verticesB[i]);

			if (_verticesA[i] != _verticesB[i])
				_edges[_verticesB[i]].push_back(_verticesA[i]);
		}

	}

	void quicksortByEdgeWeight();
	int getNumVertices();

	int getNumEdges();

	int getFirstVertexAtIndex(int index);
	int getSecondVertexAtIndex(int index);

	double getEdgeWeightAtIndex(int index);
	std::vector<int> &getEdgeListForVertex(int vertex);
private:
	int selectPivotIndex(int startIndex, int endIndex);

	int partition(int startIndex, int endIndex, int pivotIndex);
	void swapEdges(int indexOne, int indexTwo);

};

