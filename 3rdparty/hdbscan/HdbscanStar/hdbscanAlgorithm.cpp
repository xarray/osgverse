#include <iostream>
#include <limits>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include "../Utils/bitSet.hpp"
#include <list>
#include "undirectedGraph.hpp"
#include"outlierScore.hpp"
#include"cluster.hpp"
#include"hdbscanConstraint.hpp"
#include"hdbscanAlgorithm.hpp"


std::vector<double> hdbscanStar::hdbscanAlgorithm::calculateCoreDistances(std::vector<std::vector<double>> distances, int k)
{
	int length = distances.size();

	int numNeighbors = k - 1;
	std::vector<double>coreDistances(length);
	if (k == 1)
	{
		for (int point = 0; point < length; point++)
		{
			coreDistances[point] = 0;
		}
		return coreDistances;
	}
	for (int point = 0; point < length; point++)
	{
		std::vector<double> kNNDistances(numNeighbors);  //Sorted nearest distances found so far
		for (int i = 0; i < numNeighbors; i++)
		{
			kNNDistances[i] = std::numeric_limits<double>::max();
		}

		for (int neighbor = 0; neighbor < length; neighbor++)
		{
			if (point == neighbor)
				continue;
			double distance = distances[point][neighbor];
			int neighborIndex = numNeighbors;
			//Check at which position in the nearest distances the current distance would fit:
			while (neighborIndex >= 1 && distance < kNNDistances[neighborIndex - 1])
			{
				neighborIndex--;
			}
			//Shift elements in the array to make room for the current distance:
			if (neighborIndex < numNeighbors)
			{
				for (int shiftIndex = numNeighbors - 1; shiftIndex > neighborIndex; shiftIndex--)
				{
					kNNDistances[shiftIndex] = kNNDistances[shiftIndex - 1];
				}
				kNNDistances[neighborIndex] = distance;
			}

		}
		coreDistances[point] = kNNDistances[numNeighbors - 1];
	}
	return coreDistances;
}
undirectedGraph hdbscanStar::hdbscanAlgorithm::constructMst(std::vector<std::vector<double>> distances, std::vector<double> coreDistances, bool selfEdges)
{
	int length = distances.size();
	int selfEdgeCapacity = 0;
	if (selfEdges)
		selfEdgeCapacity = length;
	bitSet attachedPoints;

	std::vector<int> nearestMRDNeighbors(length - 1 + selfEdgeCapacity);
	std::vector<double> nearestMRDDistances(length - 1 + selfEdgeCapacity);

	for (int i = 0; i < length - 1; i++)
	{
		nearestMRDDistances[i] = std::numeric_limits<double>::max();
	}

	int currentPoint = length - 1;
	int numAttachedPoints = 1;
	attachedPoints.set(length - 1);

	while (numAttachedPoints < length)
	{

		int nearestMRDPoint = -1;
		double nearestMRDDistance = std::numeric_limits<double>::max();
		for (int neighbor = 0; neighbor < length; neighbor++)
		{
			if (currentPoint == neighbor)
				continue;
			if (attachedPoints.get(neighbor) == true)
				continue;
			double distance = distances[currentPoint][neighbor];
			double mutualReachabiltiyDistance = distance;
			if (coreDistances[currentPoint] > mutualReachabiltiyDistance)
				mutualReachabiltiyDistance = coreDistances[currentPoint];

			if (coreDistances[neighbor] > mutualReachabiltiyDistance)
				mutualReachabiltiyDistance = coreDistances[neighbor];

			if (mutualReachabiltiyDistance < nearestMRDDistances[neighbor])
			{
				nearestMRDDistances[neighbor] = mutualReachabiltiyDistance;
				nearestMRDNeighbors[neighbor] = currentPoint;
			}

			if (nearestMRDDistances[neighbor] <= nearestMRDDistance)
			{
				nearestMRDDistance = nearestMRDDistances[neighbor];
				nearestMRDPoint = neighbor;
			}

		}
		attachedPoints.set(nearestMRDPoint);
		numAttachedPoints++;
		currentPoint = nearestMRDPoint;
	}
	std::vector<int> otherVertexIndices(length - 1 + selfEdgeCapacity);
	for (int i = 0; i < length - 1; i++)
	{
		otherVertexIndices[i] = i;
	}
	if (selfEdges)
	{
		for (int i = length - 1; i < length * 2 - 1; i++)
		{
			int vertex = i - (length - 1);
			nearestMRDNeighbors[i] = vertex;
			otherVertexIndices[i] = vertex;
			nearestMRDDistances[i] = coreDistances[vertex];
		}
	}
	undirectedGraph undirectedGraphObject(length, nearestMRDNeighbors, otherVertexIndices, nearestMRDDistances);
	return undirectedGraphObject;

}

void hdbscanStar::hdbscanAlgorithm::computeHierarchyAndClusterTree(undirectedGraph* mst, int minClusterSize, std::vector<hdbscanConstraint> constraints, std::vector<std::vector<int>>& hierarchy, std::vector<double>& pointNoiseLevels, std::vector<int>& pointLastClusters, std::vector<cluster*>& clusters)
{
	int hierarchyPosition = 0;

	//The current edge being removed from the MST:
	int currentEdgeIndex = mst->getNumEdges() - 1;
	int nextClusterLabel = 2;
	bool nextLevelSignificant = true;

	//The previous and current cluster numbers of each point in the data set:
	std::vector<int> previousClusterLabels(mst->getNumVertices());
	std::vector<int> currentClusterLabels(mst->getNumVertices());

	for (int i = 0; i < currentClusterLabels.size(); i++)
	{
		currentClusterLabels[i] = 1;
		previousClusterLabels[i] = 1;
	}
	//std::vector<cluster *> clusters;
	clusters.push_back(NULL);
	//cluster cluster_object(1, NULL, std::numeric_limits<double>::quiet_NaN(),  mst->getNumVertices());
	clusters.push_back(new cluster(1, NULL, std::numeric_limits<double>::quiet_NaN(), mst->getNumVertices()));

	std::set<int> clusterOne;
	clusterOne.insert(1);
	calculateNumConstraintsSatisfied(
		clusterOne,
		clusters,
		constraints,
		currentClusterLabels);
	std::set<int> affectedClusterLabels;
	std::set<int> affectedVertices;
	while (currentEdgeIndex >= 0)
	{
		double currentEdgeWeight = mst->getEdgeWeightAtIndex(currentEdgeIndex);
		std::vector<cluster*> newClusters;
		while (currentEdgeIndex >= 0 && mst->getEdgeWeightAtIndex(currentEdgeIndex) == currentEdgeWeight)
		{
			int firstVertex = mst->getFirstVertexAtIndex(currentEdgeIndex);
			int secondVertex = mst->getSecondVertexAtIndex(currentEdgeIndex);
			std::vector<int>& firstVertexEdgeList = mst->getEdgeListForVertex(firstVertex);
			std::vector<int>::iterator secondVertexInFirstEdgeList = std::find(firstVertexEdgeList.begin(), firstVertexEdgeList.end(), secondVertex);
			if (secondVertexInFirstEdgeList != mst->getEdgeListForVertex(firstVertex).end())
				mst->getEdgeListForVertex(firstVertex).erase(secondVertexInFirstEdgeList);
			std::vector<int>& secondVertexEdgeList = mst->getEdgeListForVertex(secondVertex);
			std::vector<int>::iterator firstVertexInSecondEdgeList = std::find(secondVertexEdgeList.begin(), secondVertexEdgeList.end(), firstVertex);
			if (firstVertexInSecondEdgeList != mst->getEdgeListForVertex(secondVertex).end())
				mst->getEdgeListForVertex(secondVertex).erase(firstVertexInSecondEdgeList);

			if (currentClusterLabels[firstVertex] == 0)
			{
				currentEdgeIndex--;
				continue;
			}
			affectedVertices.insert(firstVertex);
			affectedVertices.insert(secondVertex);
			affectedClusterLabels.insert(currentClusterLabels[firstVertex]);
			currentEdgeIndex--;
		}
		if (!affectedClusterLabels.size())
			continue;
		while (affectedClusterLabels.size())
		{
			int examinedClusterLabel = *prev(affectedClusterLabels.end());
			affectedClusterLabels.erase(prev(affectedClusterLabels.end()));
			std::set<int> examinedVertices;
			//std::set<int>::iterator affectedIt;
			for (auto affectedIt = affectedVertices.begin(); affectedIt != affectedVertices.end();)
			{
				int vertex = *affectedIt;
				if (currentClusterLabels[vertex] == examinedClusterLabel)
				{
					examinedVertices.insert(vertex);
					affectedIt = affectedVertices.erase(affectedIt);

				}
				else {
					++affectedIt;
				}
			}
			std::set<int> firstChildCluster;
			std::list<int> unexploredFirstChildClusterPoints;
			int numChildClusters = 0;
			while (examinedVertices.size())
			{

				std::set<int> constructingSubCluster;
				int iters = 0;
				std::list<int> unexploredSubClusterPoints;
				bool anyEdges = false;
				bool incrementedChildCount = false;
				int rootVertex = *prev(examinedVertices.end());
				constructingSubCluster.insert(rootVertex);
				unexploredSubClusterPoints.push_back(rootVertex);
				examinedVertices.erase(prev(examinedVertices.end()));
				while (unexploredSubClusterPoints.size())
				{
					int vertexToExplore = *unexploredSubClusterPoints.begin();
					unexploredSubClusterPoints.erase(unexploredSubClusterPoints.begin());
					std::vector<int>& vertexToExploreEdgeList = mst->getEdgeListForVertex(vertexToExplore);
					for (std::vector<int>::iterator it = vertexToExploreEdgeList.begin(); it != vertexToExploreEdgeList.end();)
					{
						int neighbor = *it;
						anyEdges = true;
						if (std::find(constructingSubCluster.begin(), constructingSubCluster.end(), neighbor) == constructingSubCluster.end())
						{
							constructingSubCluster.insert(neighbor);
							unexploredSubClusterPoints.push_back(neighbor);
							if (std::find(examinedVertices.begin(), examinedVertices.end(), neighbor) != examinedVertices.end())
								examinedVertices.erase(std::find(examinedVertices.begin(), examinedVertices.end(), neighbor));

						}
						else {
							++it;
						}
					}
					if (!incrementedChildCount && constructingSubCluster.size() >= minClusterSize && anyEdges)
					{
						incrementedChildCount = true;
						numChildClusters++;

						//If this is the first valid child cluster, stop exploring it:
						if (firstChildCluster.size() == 0)
						{
							firstChildCluster = constructingSubCluster;
							unexploredFirstChildClusterPoints = unexploredSubClusterPoints;
							break;
						}
					}

				}
				//If there could be a split, and this child cluster is valid:
				if (numChildClusters >= 2 && constructingSubCluster.size() >= minClusterSize && anyEdges)
				{
					//Check this child cluster is not equal to the unexplored first child cluster:
					int firstChildClusterMember = *prev(firstChildCluster.end());
					if (std::find(constructingSubCluster.begin(), constructingSubCluster.end(), firstChildClusterMember) != constructingSubCluster.end())
						numChildClusters--;
					//Otherwise, c a new cluster:
					else
					{
						cluster* newCluster = createNewCluster(constructingSubCluster, currentClusterLabels,
							clusters[examinedClusterLabel], nextClusterLabel, currentEdgeWeight);
						newClusters.push_back(newCluster);
						clusters.push_back(newCluster);
						nextClusterLabel++;
					}
				}
				else if (constructingSubCluster.size() < minClusterSize || !anyEdges)
				{
					createNewCluster(constructingSubCluster, currentClusterLabels,
						clusters[examinedClusterLabel], 0, currentEdgeWeight);

					for (std::set<int>::iterator it = constructingSubCluster.begin(); it != constructingSubCluster.end(); it++)
					{
						int point = *it;
						pointNoiseLevels[point] = currentEdgeWeight;
						pointLastClusters[point] = examinedClusterLabel;
					}
				}
			}
			if (numChildClusters >= 2 && currentClusterLabels[*firstChildCluster.begin()] == examinedClusterLabel)
			{
				while (unexploredFirstChildClusterPoints.size())
				{
					int vertexToExplore = *unexploredFirstChildClusterPoints.begin();
					unexploredFirstChildClusterPoints.pop_front();
					for (std::vector<int>::iterator it = mst->getEdgeListForVertex(vertexToExplore).begin(); it != mst->getEdgeListForVertex(vertexToExplore).end(); it++)
					{
						int neighbor = *it;
						if (std::find(firstChildCluster.begin(), firstChildCluster.end(), neighbor) == firstChildCluster.end())
						{
							firstChildCluster.insert(neighbor);
							unexploredFirstChildClusterPoints.push_back(neighbor);
						}
					}
				}
				cluster* newCluster = createNewCluster(firstChildCluster, currentClusterLabels,
					clusters[examinedClusterLabel], nextClusterLabel, currentEdgeWeight);
				newClusters.push_back(newCluster);
				clusters.push_back(newCluster);
				nextClusterLabel++;
			}
		}
		if (nextLevelSignificant || newClusters.size())
		{
			std::vector<int> lineContents(previousClusterLabels.size());
			for (int i = 0; i < previousClusterLabels.size(); i++)
				lineContents[i] = previousClusterLabels[i];
			hierarchy.push_back(lineContents);
			hierarchyPosition++;
		}
		std::set<int> newClusterLabels;
		for (std::vector<cluster*>::iterator it = newClusters.begin(); it != newClusters.end(); it++)
		{
			cluster* newCluster = *it;
			newCluster->HierarchyPosition = hierarchyPosition;
			newClusterLabels.insert(newCluster->Label);
		}
		if (newClusterLabels.size())
			calculateNumConstraintsSatisfied(newClusterLabels, clusters, constraints, currentClusterLabels);

		for (int i = 0; i < previousClusterLabels.size(); i++)
		{
			previousClusterLabels[i] = currentClusterLabels[i];
		}
		if (!newClusters.size())
			nextLevelSignificant = false;
		else
			nextLevelSignificant = true;
	}

	{
		std::vector<int> lineContents(previousClusterLabels.size() + 1);
		for (int i = 0; i < previousClusterLabels.size(); i++)
			lineContents[i] = 0;
		hierarchy.push_back(lineContents);
	}
}
std::vector<int> hdbscanStar::hdbscanAlgorithm::findProminentClusters(std::vector<cluster*>& clusters, std::vector<std::vector<int>>& hierarchy, int numPoints)
{
	//Take the list of propagated clusters from the root cluster:
	std::vector<cluster*> solution = clusters[1]->PropagatedDescendants;
	std::vector<int> flatPartitioning(numPoints);

	//Store all the hierarchy positions at which to find the birth points for the flat clustering:
	std::map<int, std::vector<int>> significantHierarchyPositions;

	std::vector<cluster*>::iterator it = solution.begin();
	while (it != solution.end())
	{
		int hierarchyPosition = (*it)->HierarchyPosition;
		if (significantHierarchyPositions.count(hierarchyPosition) > 0)
			significantHierarchyPositions[hierarchyPosition].push_back((*it)->Label);
		else
			significantHierarchyPositions[hierarchyPosition].push_back((*it)->Label);
		it++;
	}

	//Go through the hierarchy file, setting labels for the flat clustering:
	while (significantHierarchyPositions.size())
	{
		std::map<int, std::vector<int>>::iterator entry = significantHierarchyPositions.begin();
		std::vector<int> clusterList = entry->second;
		int hierarchyPosition = entry->first;
		significantHierarchyPositions.erase(entry->first);

		std::vector<int> lineContents = hierarchy[hierarchyPosition];

		for (int i = 0; i < lineContents.size(); i++)
		{
			int label = lineContents[i];
			if (std::find(clusterList.begin(), clusterList.end(), label) != clusterList.end())
				flatPartitioning[i] = label;
		}
	}
	return flatPartitioning;
}
std::vector<double> hdbscanStar::hdbscanAlgorithm::findMembershipScore(std::vector<int> clusterids, std::vector<double> coreDistances)
{
	
	int length = clusterids.size();
	std::vector<double> prob(length, std::numeric_limits<double>::max());
	int i=0;
	
	while(i<length)
	{
		if(prob[i]==std::numeric_limits<double>::max())
		{
			
			int clusterno = clusterids[i];
			std::vector<int>::iterator iter = clusterids.begin()+i;
			std::vector<int> indices;
			while ((iter = std::find(iter, clusterids.end(), clusterno)) != clusterids.end())
			{
				
				indices.push_back(distance(clusterids.begin(), iter));
				iter++;
				if(iter==clusterids.end())
					break;

			}
			if(clusterno==0)
			{
				for(int j=0; j<indices.size();j++)
				{
					prob[indices[j]] = 0;
				}
				i++;
				continue;
			}
			std::vector<double> tempCoreDistances(indices.size());
			for(int j=0; j<indices.size();j++)
			{
				tempCoreDistances[j] = coreDistances[j];
			}
			double maxCoreDistance = *max_element(tempCoreDistances.begin(), tempCoreDistances.end());
			for(int j=0; j<tempCoreDistances.size();j++)
			{
				prob[indices[j]] = ( maxCoreDistance - tempCoreDistances[j] ) / maxCoreDistance;
			}

		}
		
		i++;
	}
	return prob;
	
}

bool hdbscanStar::hdbscanAlgorithm::propagateTree(std::vector<cluster*>& clusters)
{
	std::map<int, cluster*> clustersToExamine;
	bitSet addedToExaminationList;
	bool infiniteStability = false;

	//Find all leaf clusters in the cluster tree:
	for (cluster* cluster : clusters)
	{
		if (cluster != NULL && !cluster->HasChildren)
		{
			int label = cluster->Label;
			clustersToExamine.erase(label);
			clustersToExamine.insert({ label, cluster });
			addedToExaminationList.set(label);
		}
	}
	//Iterate through every cluster, propagating stability from children to parents:
	while (clustersToExamine.size())
	{
		std::map<int, cluster*>::iterator currentKeyValue = prev(clustersToExamine.end());
		cluster* currentCluster = currentKeyValue->second;
		clustersToExamine.erase(currentKeyValue->first);
		currentCluster->propagate();

		if (currentCluster->Stability == std::numeric_limits<double>::infinity())
			infiniteStability = true;

		if (currentCluster->Parent != NULL)
		{
			cluster* parent = currentCluster->Parent;
			int label = parent->Label;

			if (!addedToExaminationList.get(label))
			{
				clustersToExamine.erase(label);
				clustersToExamine.insert({ label, parent });
				addedToExaminationList.set(label);
			}
		}
	}

	return infiniteStability;
}

/// <summary>
/// Produces the outlier score for each point in the data set, and returns a sorted list of outlier
/// scores.  propagateTree() must be called before calling this method.
/// </summary>
/// <param name="clusters">A list of Clusters forming a cluster tree which has already been propagated</param>
/// <param name="pointNoiseLevels">A double[] with the levels at which each point became noise</param>
/// <param name="pointLastClusters">An int[] with the last label each point had before becoming noise</param>
/// <param name="coreDistances">An array of core distances for each data point</param>
/// <returns>An List of OutlierScores, sorted in descending order</returns>
std::vector<outlierScore> hdbscanStar::hdbscanAlgorithm::calculateOutlierScores(
	std::vector<cluster*>& clusters,
	std::vector<double>& pointNoiseLevels,
	std::vector<int>& pointLastClusters,
	std::vector<double> coreDistances)
{
	int numPoints = pointNoiseLevels.size();
	std::vector<outlierScore> outlierScores;

	//Iterate through each point, calculating its outlier score:
	for (int i = 0; i < numPoints; i++)
	{
		double epsilonMax = clusters[pointLastClusters[i]]->PropagatedLowestChildDeathLevel;
		double epsilon = pointNoiseLevels[i];
		double score = 0;

		if (epsilon != 0)
			score = 1 - (epsilonMax / epsilon);

		outlierScores.push_back(outlierScore(score, coreDistances[i], i));
	}
	//Sort the outlier scores:
	sort(outlierScores.begin(), outlierScores.end());

	return outlierScores;
}

/// <summary>
/// Removes the set of points from their parent Cluster, and creates a new Cluster, provided the
/// clusterId is not 0 (noise).
/// </summary>
/// <param name="points">The set of points to be in the new Cluster</param>
/// <param name="clusterLabels">An array of cluster labels, which will be modified</param>
/// <param name="parentCluster">The parent Cluster of the new Cluster being created</param>
/// <param name="clusterLabel">The label of the new Cluster </param>
/// <param name="edgeWeight">The edge weight at which to remove the points from their previous Cluster</param>
/// <returns>The new Cluster, or null if the clusterId was 0</returns>
cluster* hdbscanStar::hdbscanAlgorithm::createNewCluster(
	std::set<int>& points,
	std::vector<int>& clusterLabels,
	cluster* parentCluster,
	int clusterLabel,
	double edgeWeight)
{
	std::set<int>::iterator it = points.begin();
	while (it != points.end())
	{
		clusterLabels[*it] = clusterLabel;
		++it;
	}
	parentCluster->detachPoints(points.size(), edgeWeight);

	if (clusterLabel != 0)
	{
		return new cluster(clusterLabel, parentCluster, edgeWeight, points.size());
	}

	parentCluster->addPointsToVirtualChildCluster(points);
	return NULL;
}
/// <summary>
/// Calculates the number of constraints satisfied by the new clusters and virtual children of the
/// parents of the new clusters.
/// </summary>
/// <param name="newClusterLabels">Labels of new clusters</param>
/// <param name="clusters">An List of clusters</param>
/// <param name="constraints">An List of constraints</param>
/// <param name="clusterLabels">An array of current cluster labels for points</param>
void hdbscanStar::hdbscanAlgorithm::calculateNumConstraintsSatisfied(
	std::set<int>& newClusterLabels,
	std::vector<cluster*>& clusters,
	std::vector<hdbscanConstraint>& constraints,
	std::vector<int>& clusterLabels)
{

	if (constraints.size() == 0)
		return;

	std::vector<cluster> parents;
	std::vector<cluster> ::iterator it;
	for (int label : newClusterLabels)
	{
		cluster* parent = clusters[label]->Parent;
		if (parent != NULL && !(find(parents.begin(), parents.end(), *parent) != parents.end()))
			parents.push_back(*parent);
	}

	for (hdbscanConstraint constraint : constraints)
	{
		int labelA = clusterLabels[constraint.getPointA()];
		int labelB = clusterLabels[constraint.getPointB()];

		if (constraint.getConstraintType() == hdbscanConstraintType::mustLink && labelA == labelB)
		{
			if (find(newClusterLabels.begin(), newClusterLabels.end(), labelA) != newClusterLabels.end())
				clusters[labelA]->addConstraintsSatisfied(2);
		}
		else if (constraint.getConstraintType() == hdbscanConstraintType::cannotLink && (labelA != labelB || labelA == 0))
		{
			if (labelA != 0 && find(newClusterLabels.begin(), newClusterLabels.end(), labelA) != newClusterLabels.end())
				clusters[labelA]->addConstraintsSatisfied(1);
			if (labelB != 0 && (find(newClusterLabels.begin(), newClusterLabels.end(), labelA) != newClusterLabels.end()))
				clusters[labelB]->addConstraintsSatisfied(1);
			if (labelA == 0)
			{
				for (cluster parent : parents)
				{
					if (parent.virtualChildClusterConstraintsPoint(constraint.getPointA()))
					{
						parent.addVirtualChildConstraintsSatisfied(1);
						break;
					}
				}
			}
			if (labelB == 0)
			{
				for (cluster parent : parents)
				{
					if (parent.virtualChildClusterConstraintsPoint(constraint.getPointB()))
					{
						parent.addVirtualChildConstraintsSatisfied(1);
						break;
					}
				}
			}
		}
	}

	for (cluster parent : parents)
	{
		parent.releaseVirtualChildCluster();
	}
}
