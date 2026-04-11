//  Title = { PAIRWISE LINKAGE FOR POINT CLOUD SEGMENTATION },
//  Author = { Lu, Xiaohu and Yao, Jian and Tu, Jinge and Li, Kai and Li, Li and Liu, Yahui },
//  Journal = { ISPRS Annals of Photogrammetry, Remote Sensing \& Spatial Information Sciences },
//  Year = { 2016 }

#ifndef _CLUSTER_GROW_PLINKAGE_H_
#define _CLUSTER_GROW_PLINKAGE_H_
#pragma once

#include "PCAFunctions.h"
#include <Eigen/Core>

class ClusterGrowPLinkage 
{
public:
	ClusterGrowPLinkage( int k, double theta, PLANE_MODE planeMode);
	~ClusterGrowPLinkage();

	void run( std::vector<std::vector<int> > &clusters );

	void setData(PointCloud<double> &data, std::vector<PCAInfo> &pcaInfos);

	void createLinkage( std::vector<PCAInfo> &pcaInfos, std::vector<int> &clusterCenterIdx, 
		std::vector<std::vector<int> > &singleLinkage );

	void clustering( std::vector<PCAInfo> &pcaInfos, std::vector<int> &clusterCenterIdx, 
		std::vector<std::vector<int> > &singleLinkage, std::vector<std::vector<int> > &clusters );

	void createPatch( std::vector<std::vector<int> > &clusters, std::vector<PCAInfo> &patches );

	void patchMerging( std::vector<PCAInfo> &patches, std::vector<PCAInfo> &pcaInfos,
		std::vector<std::vector<int> > &clusters );

	double meadian( std::vector<double> &dataset );

private:
	int k;
	double theta;
	PLANE_MODE planeMode;

	int pointNum;
	PointCloud<double> pointData;
	std::vector<PCAInfo> pcaInfos;
};

#endif // _CLUSTER_GROW_PLINKAGE_H_
