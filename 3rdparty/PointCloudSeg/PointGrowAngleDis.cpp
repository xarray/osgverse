#include "PointGrowAngleDis.h"
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

PointGrowAngleDis::PointGrowAngleDis( double theta, int Rmin )
{
	this->theta = theta;
	this->Rmin = Rmin;
}

PointGrowAngleDis::~PointGrowAngleDis()
{
}

void PointGrowAngleDis::setData(PointCloud<double> &data, std::vector<PCAInfo> &infos)
{
	this->pointData = data;
	this->pointNum = data.pts.size();
	this->pcaInfos = infos;
}

void PointGrowAngleDis::run( std::vector<std::vector<int> > &clusters )
{
	double b = 1.4826;

	// sort the data points according to their curvature
	std::vector<std::pair<int,double> > idxSorted( this->pointNum );
	for ( int i=0; i<this->pointNum; ++i )
	{
		idxSorted[i].first = i;
		idxSorted[i].second = pcaInfos[i].lambda0;
	}
	std::sort( idxSorted.begin(), idxSorted.end(), [](const std::pair<int,double>& lhs, const std::pair<int,double>& rhs) { return lhs.second < rhs.second; } );

	// begin region growing
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
		std::vector<int> clusterNew;
		clusterNew.push_back( idxSorted[i].first );
		Eigen::Vector3d normalStart = pcaInfos[idxSorted[i].first].normal;

		int count = 0;
		while( count < clusterNew.size() )
		{
			int idxSeed = clusterNew[count];
			int num = pcaInfos[idxSeed].idxIn.size();
			Eigen::Vector3d normalSeed = pcaInfos[idxSeed].normal;

			// EDth
			std::vector<double> EDs( num );
			for ( int j=0; j<num; ++j )
			{
				int idx = pcaInfos[idxSeed].idxIn[j];
				double dx = this->pointData.pts[idxSeed].x - this->pointData.pts[idx].x;
				double dy = this->pointData.pts[idxSeed].y - this->pointData.pts[idx].y;
				double dz = this->pointData.pts[idxSeed].z - this->pointData.pts[idx].z;

				EDs[j] = sqrt( dx * dx + dy * dy + dz * dz );
			}
			std::sort( EDs.begin(), EDs.end(), [](const double& lhs, const double& rhs) { return lhs < rhs; } );
			double EDth = EDs[ EDs.size() / 2 ];

			// ODth
			Eigen::Vector3d h_mean( 0, 0, 0 );
			for( int j = 0; j < num; ++j )
			{
				int idx = pcaInfos[idxSeed].idxIn[j];
				h_mean += Eigen::Vector3d(this->pointData.pts[idx].x, this->pointData.pts[idx].y, this->pointData.pts[idx].z);
			}
			h_mean *= ( 1.0 / num );

			std::vector<double> ODs( num );
			for( int j = 0; j < num; ++j )
			{
				int idx = pcaInfos[idxSeed].idxIn[j];
				Eigen::Vector3d pt(this->pointData.pts[idx].x, this->pointData.pts[idx].y, this->pointData.pts[idx].z);
				double OD = fabs( ( pt - h_mean ).dot( pcaInfos[idxSeed].normal ) );
				ODs[j] = OD;
			}

			// calculate the Rz-score for all points using ODs
			std::vector<double> sorted_ODs( ODs.begin(), ODs.end() );
			double median_OD =  meadian( sorted_ODs );
			std::vector<double>().swap( sorted_ODs );

			std::vector<double> abs_diff_ODs( num );
			for( int j = 0; j < num; ++j )
			{
				abs_diff_ODs[j] = fabs( ODs[j] - median_OD );
			}
			double MAD_OD = b * meadian( abs_diff_ODs );
			double ODth = median_OD + 2.0 * MAD_OD;

			// point cloud collection
			for( int j = 0; j < num; ++j )
			{
				int idx = pcaInfos[idxSeed].idxIn[j];
				if ( used[idx] )
				{
					continue;
				}

				if ( ODs[j] < ODth && EDs[j] < EDth )
				{
					Eigen::Vector3d normalCur = pcaInfos[idx].normal;
					double angle = acos( normalCur.dot(normalStart) );
					if (angle != angle)
					{
						continue;
					}

					if ( min( angle, M_PI - angle ) < this->theta )
					{
						clusterNew.push_back( idx );
						used[idx] = 1;
					}
				}
			}

			count ++;
		}

		if ( clusterNew.size() > this->Rmin )
		{
			clusters.push_back( clusterNew );
		}
	}

	cout<<" number of clusters : "<<clusters.size()<<endl;
}

double PointGrowAngleDis::meadian( std::vector<double> &dataset )
{
	std::sort( dataset.begin(), dataset.end(), []( const double& lhs, const double& rhs ){ return lhs < rhs; } );

	return dataset[dataset.size()/2];
}
