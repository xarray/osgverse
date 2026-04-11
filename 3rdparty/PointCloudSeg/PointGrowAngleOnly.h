//  Title   = {Segmentation of point clouds using smoothness constraint},
//  Author  = {Rabbani, Tahir and van den Heuvel, Frank and Vosselmann, G},
//  Journal = {International Archives of Photogrammetry, Remote Sensing and Spatial Information Sciences},
//  Year    = {2006}

#ifndef _POINT_GROW_ANGLE_ONLY_H_
#define _POINT_GROW_ANGLE_ONLY_H_
#pragma once

#include "PCAFunctions.h"
#include <Eigen/Core>

class PointGrowAngleOnly 
{
public:
	PointGrowAngleOnly( double theta, double percent );
	~PointGrowAngleOnly();

	void run( std::vector<std::vector<int> > &clusters );

	void setData(PointCloud<double> &data, std::vector<PCAInfo> &pcaInfos);

private:
	 double theta;
	 double percent;

	 int pointNum;
	 PointCloud<double> pointData;
	 std::vector<PCAInfo> pcaInfos;
};

#endif // _POINT_GROW_ANGLE_ONLY_H_
