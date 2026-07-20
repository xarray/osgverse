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

namespace hdbscanStar
{
	class hdbscanAlgorithm
	{
	public:
		/// <summary>
		/// Calculates the core distances for each point in the data set, given some value for k.
		/// </summary>
		/// <param name="distances">A vector of vectors where index [i][j] indicates the jth attribute of data point i</param>
		/// <param name="k">Each point's core distance will be it's distance to the kth nearest neighbor</param>
		/// <returns> An array of core distances</returns>
		static std::vector<double> calculateCoreDistances(std::vector<std::vector<double>> distances, int k);
		
		static undirectedGraph constructMst(std::vector<std::vector<double>> distances, std::vector<double> coreDistances, bool selfEdges);
		
	
		/// <summary>
		/// Propagates constraint satisfaction, stability, and lowest child death level from each child
		/// cluster to each parent cluster in the tree.  This method must be called before calling
		/// findProminentClusters() or calculateOutlierScores().
		/// </summary>
		/// <param name="clusters">A list of Clusters forming a cluster tree</param>
		/// <returns>true if there are any clusters with infinite stability, false otherwise</returns>


		static void computeHierarchyAndClusterTree(undirectedGraph *mst, int minClusterSize, std::vector<hdbscanConstraint> constraints, std::vector<std::vector<int>> &hierarchy, std::vector<double> &pointNoiseLevels, std::vector<int> &pointLastClusters, std::vector<cluster*> &clusters);
		
		static std::vector<int> findProminentClusters(std::vector<cluster*> &clusters, std::vector<std::vector<int>> &hierarchy, int numPoints);

		static std::vector<double> findMembershipScore(std::vector<int> clusterids, std::vector<double> coreDistances);
		
		static bool propagateTree(std::vector<cluster*> &sclusters);
		
		/// <summary>
		/// Produces the outlier score for each point in the data set, and returns a sorted list of outlier
		/// scores.  propagateTree() must be called before calling this method.
		/// </summary>
		/// <param name="clusters">A list of Clusters forming a cluster tree which has already been propagated</param>
		/// <param name="pointNoiseLevels">A double[] with the levels at which each point became noise</param>
		/// <param name="pointLastClusters">An int[] with the last label each point had before becoming noise</param>
		/// <param name="coreDistances">An array of core distances for each data point</param>
		/// <returns>An List of OutlierScores, sorted in descending order</returns>
		static std::vector<outlierScore> calculateOutlierScores(
			std::vector<cluster*> &clusters,
			std::vector<double> &pointNoiseLevels,
			std::vector<int> &pointLastClusters,
			std::vector<double> coreDistances);
		
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
		static cluster* createNewCluster(
			std::set<int>& points,
			std::vector<int> &clusterLabels,
			cluster *parentCluster,
			int clusterLabel,
			double edgeWeight);
		
		/// <summary>
		/// Calculates the number of constraints satisfied by the new clusters and virtual children of the
		/// parents of the new clusters.
		/// </summary>
		/// <param name="newClusterLabels">Labels of new clusters</param>
		/// <param name="clusters">An List of clusters</param>
		/// <param name="constraints">An List of constraints</param>
		/// <param name="clusterLabels">An array of current cluster labels for points</param>
		static void calculateNumConstraintsSatisfied(
			std::set<int>& newClusterLabels,
			std::vector<cluster*>& clusters,
			std::vector<hdbscanConstraint>& constraints,
			std::vector<int>& clusterLabels);
		
	};

}


