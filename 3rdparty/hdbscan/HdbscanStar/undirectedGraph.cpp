#include "undirectedGraph.hpp"

void undirectedGraph::quicksortByEdgeWeight()
{
	int _edgeWeightsLength = _edgeWeights.size();
	if (_edgeWeightsLength <= 1)
		return;

	std::vector<int> startIndexStack(_edgeWeightsLength / 2);
	std::vector<int> endIndexStack(_edgeWeightsLength / 2);

	startIndexStack[0] = 0;
	endIndexStack[0] = _edgeWeightsLength - 1;

	int stackTop = 0;
	while (stackTop >= 0)
	{
		int startIndex = startIndexStack[stackTop];
		int endIndex = endIndexStack[stackTop];
		stackTop--;
		int pivotIndex = selectPivotIndex(startIndex, endIndex);
		pivotIndex = partition(startIndex, endIndex, pivotIndex);
		if (pivotIndex > startIndex + 1)
		{
			startIndexStack[stackTop + 1] = startIndex;
			endIndexStack[stackTop + 1] = pivotIndex - 1;
			stackTop++;
		}
		if (pivotIndex < endIndex - 1)
		{
			startIndexStack[stackTop + 1] = pivotIndex + 1;
			endIndexStack[stackTop + 1] = endIndex;
			stackTop++;
		}

	}
}
int undirectedGraph::selectPivotIndex(int startIndex, int endIndex)
{
	if (startIndex - endIndex <= 1)
		return startIndex;

	double first = _edgeWeights[startIndex];
	double middle = _edgeWeights[startIndex + (endIndex - startIndex) / 2];
	double last = _edgeWeights[endIndex];

	if (first <= middle)
	{
		if (middle <= last)
			return startIndex + (endIndex - startIndex) / 2;

		if (last >= first)
			return endIndex;

		return startIndex;
	}

	if (first <= last)
		return startIndex;

	if (last >= middle)
		return endIndex;

	return startIndex + (endIndex - startIndex) / 2;
}

int undirectedGraph::partition(int startIndex, int endIndex, int pivotIndex)
{
	double pivotValue = _edgeWeights[pivotIndex];
	swapEdges(pivotIndex, endIndex);
	int lowIndex = startIndex;
	for (int i = startIndex; i < endIndex; i++)
	{
		if (_edgeWeights[i] < pivotValue)
		{
			swapEdges(i, lowIndex);
			lowIndex++;
		}
	}
	swapEdges(lowIndex, endIndex);
	return lowIndex;
}

void undirectedGraph::swapEdges(int indexOne, int indexTwo)
{
	if (indexOne == indexTwo)
		return;

	int tempVertexA = _verticesA[indexOne];
	int tempVertexB = _verticesB[indexOne];
	double tempEdgeDistance = _edgeWeights[indexOne];
	_verticesA[indexOne] = _verticesA[indexTwo];
	_verticesB[indexOne] = _verticesB[indexTwo];
	_edgeWeights[indexOne] = _edgeWeights[indexTwo];
	_verticesA[indexTwo] = tempVertexA;
	_verticesB[indexTwo] = tempVertexB;
	_edgeWeights[indexTwo] = tempEdgeDistance;
}

int undirectedGraph::getNumVertices()
{
	return _numVertices;
}

int undirectedGraph::getNumEdges()
{
	return _edgeWeights.size();
}

int undirectedGraph::getFirstVertexAtIndex(int index)
{
	return _verticesA[index];
}

int undirectedGraph::getSecondVertexAtIndex(int index)
{
	return _verticesB[index];
}

double undirectedGraph::getEdgeWeightAtIndex(int index)
{
	return _edgeWeights[index];
}

std::vector<int>& undirectedGraph::getEdgeListForVertex(int vertex)
{
	return _edges[vertex];
}