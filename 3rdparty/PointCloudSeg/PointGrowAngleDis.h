//  Add point-to-plane distance to constrict the angle-only point growing algorithm

#ifndef _POINT_GROW_ANGLE_DIS_H_
#define _POINT_GROW_ANGLE_DIS_H_
#pragma once

#include "PCAFunctions.h"
#include <Eigen/Core>

class PointGrowAngleDis 
{
public:
	PointGrowAngleDis( double theta, int Rmin );
	~PointGrowAngleDis();

	void run( std::vector<std::vector<int> > &clusters );

	void setData(PointCloud<double> &data, std::vector<PCAInfo> &pcaInfos);

	double meadian( std::vector<double> &dataset );

private:
	double theta; 
	int Rmin;

	int pointNum;
	PointCloud<double> pointData;
	std::vector<PCAInfo> pcaInfos;
};

#endif // _POINT_GROW_ANGLE_DIS_H_
