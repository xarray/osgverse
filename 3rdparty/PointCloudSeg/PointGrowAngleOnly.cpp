#include "PointGrowAngleOnly.h"
#include <fstream>
#include <stdio.h>
#include <cmath>
#ifdef ENABLE_OPENMP
#   include <omp.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;

PointGrowAngleOnly::PointGrowAngleOnly( double theta, double percent )
{
	this->theta = theta;
	this->percent = percent;
}

PointGrowAngleOnly::~PointGrowAngleOnly()
{
}

void PointGrowAngleOnly::setData(PointCloud<double> &data, std::vector<PCAInfo> &infos)
{
	this->pointData = data;
	this->pointNum = data.pts.size();
	this->pcaInfos = infos;
}


void PointGrowAngleOnly::run( std::vector<std::vector<int> > &clusters )
{
	// residual threshold
	std::vector<std::pair<int,double> > idxSorted( this->pointNum );
	for ( int i=0; i<this->pointNum; ++i )
	{
		idxSorted[i].first = i;
		idxSorted[i].second = pcaInfos[i].lambda0;
	}
	std::sort( idxSorted.begin(), idxSorted.end(), [](const std::pair<int,double>& lhs, const std::pair<int,double>& rhs) { return lhs.second < rhs.second; } );
	double Rth = idxSorted[this->pointNum * this->percent].second;

	// smooth constraint region growing
	std::vector<int> used( this->pointNum, 0 );
	for ( int i=0; i<this->pointNum; ++i )
	{
		if ( used[i] )
		{
			continue;
		}

		if ( i % 10000 == 0 )
		{
			cout<<i<<endl;
		}

		//
		std::vector<int> clusterIdx;
		clusterIdx.push_back( idxSorted[i].first );

		std::vector<int> seedIdx;
		seedIdx.push_back( idxSorted[i].first );

		int count = 0;
		while( count < seedIdx.size() )
		{
			int idxSeed = seedIdx[count];
			int num = pcaInfos[idxSeed].idxAll.size();
			Eigen::Vector3d normalSeed = pcaInfos[idxSeed].normal;

			// point cloud collection
			for( int j = 0; j < num; ++j )
			{
				int idx = pcaInfos[idxSeed].idxAll[j];
				if ( used[idx] )
				{
					continue;
				}

				Eigen::Vector3d normalCur = pcaInfos[idx].normal;
				double angle = acos( normalCur.dot(normalSeed) );
				if ( min( angle, M_PI - angle ) < this->theta )
				{
					clusterIdx.push_back( idx );
					used[idx] = 1;
					if ( pcaInfos[idx].lambda0 < Rth )
					{
						seedIdx.push_back( idx );
					}
				}
			}

			count ++;
		}

		if ( clusterIdx.size() > 10 )
		{
			clusters.push_back( clusterIdx );
		}
	}

	cout<<" number of clusters : "<<clusters.size()<<endl;
}
