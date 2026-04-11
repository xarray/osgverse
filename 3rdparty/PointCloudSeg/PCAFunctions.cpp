#include "PCAFunctions.h"
#include <stdio.h>
#include <cmath>
#ifdef ENABLE_OPENMP
#   include <omp.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;

void PCAFunctions::PCA(PointCloud<double> &cloud, int k, std::vector<PCAInfo> &pcaInfos)
{
	cout << "building kd-tree ..." << endl;
	double MINVALUE = 1e-7;
	int pointNum = cloud.pts.size();
	double scale = 0.0, magnitd = 0.0;

	// 1. build kd-tree
	typedef KDTreeSingleIndexAdaptor< L2_Simple_Adaptor<double, PointCloud<double> >, PointCloud<double>, 3/*dim*/ > my_kd_tree_t;
	my_kd_tree_t index(3 /*dim*/, cloud, KDTreeSingleIndexAdaptorParams(10 /* max leaf */));
	index.buildIndex();

	// 2. knn search
	size_t *out_ks = new size_t[pointNum];
	size_t **out_indices = new size_t *[pointNum];
#pragma omp parallel for
	for (int i = 0; i<pointNum; ++i)
	{
		double *query_pt = new double[3];
		query_pt[0] = cloud.pts[i].x;  query_pt[1] = cloud.pts[i].y;  query_pt[2] = cloud.pts[i].z;
		double *dis_temp = new double[k];
		out_indices[i] = new size_t[k];

		nanoflann::KNNResultSet<double> resultSet(k);
		resultSet.init(out_indices[i], dis_temp);
		index.findNeighbors(resultSet, &query_pt[0], nanoflann::SearchParams(10));
		out_ks[i] = resultSet.size();

		delete[] query_pt;
		delete[] dis_temp;
	}
	index.freeIndex(index);

	cout << "pca normal calculation ..." << endl;

	// 3. PCA normal estimation
	scale = 0.0;
	pcaInfos.resize(pointNum);
#pragma omp parallel for
	for (int i = 0; i < pointNum; ++i)
	{
		// 
		int ki = out_ks[i];

		double h_mean_x = 0.0, h_mean_y = 0.0, h_mean_z = 0.0;
		for (int j = 0; j < ki; ++j)
		{
			int idx = out_indices[i][j];
			h_mean_x += cloud.pts[idx].x;
			h_mean_y += cloud.pts[idx].y;
			h_mean_z += cloud.pts[idx].z;
		}
		h_mean_x *= 1.0 / ki;  h_mean_y *= 1.0 / ki; h_mean_z *= 1.0 / ki;

		double h_cov_1 = 0.0, h_cov_2 = 0.0, h_cov_3 = 0.0;
		double h_cov_5 = 0.0, h_cov_6 = 0.0;
		double h_cov_9 = 0.0;
		double dx = 0.0, dy = 0.0, dz = 0.0;
		for (int j = 0; j < k; ++j)
		{
			int idx = out_indices[i][j];
			dx = cloud.pts[idx].x - h_mean_x;
			dy = cloud.pts[idx].y - h_mean_y;
			dz = cloud.pts[idx].z - h_mean_z;

			h_cov_1 += dx * dx; h_cov_2 += dx * dy; h_cov_3 += dx * dz;
			h_cov_5 += dy * dy; h_cov_6 += dy * dz;
			h_cov_9 += dz * dz;
		}
		Eigen::Matrix3d h_cov;
		h_cov << h_cov_1, h_cov_2, h_cov_3,
		         h_cov_2, h_cov_5, h_cov_6,
		         h_cov_3, h_cov_6, h_cov_9;
		h_cov *= 1.0 / ki;

		// eigenvector using Eigen
		Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> eigensolver(h_cov);
		Eigen::Vector3d h_cov_evals = eigensolver.eigenvalues();
		Eigen::Matrix3d h_cov_evectors = eigensolver.eigenvectors();

		// 
		pcaInfos[i].idxAll.resize(ki);
		for (int j = 0; j<ki; ++j)
		{
			int idx = out_indices[i][j];
			pcaInfos[i].idxAll[j] = idx;
		}

		int idx = out_indices[i][3];
		dx = cloud.pts[idx].x - cloud.pts[i].x;
		dy = cloud.pts[idx].y - cloud.pts[i].y;
		dz = cloud.pts[idx].z - cloud.pts[i].z;
		double scaleTemp = sqrt(dx*dx + dy * dy + dz * dz);
		pcaInfos[i].scale = scaleTemp;
		scale += scaleTemp;

		//pcaInfos[i].lambda0 = h_cov_evals(2);
		double t = h_cov_evals(0) + h_cov_evals(1) + h_cov_evals(2) + (rand() % 10 + 1) * MINVALUE;
		pcaInfos[i].lambda0 = h_cov_evals(2) / t;
		pcaInfos[i].normal = h_cov_evectors.col(2);

		// todo, outliers removal via MCMD
		pcaInfos[i].idxIn = pcaInfos[i].idxAll;

		delete out_indices[i];
	}
	delete[] out_indices;
	delete[] out_ks;

	scale /= pointNum;
	magnitd = sqrt(cloud.pts[0].x*cloud.pts[0].x + cloud.pts[0].y*cloud.pts[0].y + cloud.pts[0].z*cloud.pts[0].z);
}

void PCAFunctions::RDPCA(PointCloud<double>& cloud, int k, std::vector<PCAInfo>& pcaInfos)
{
	// removed, bacause it's too slow
}

void PCAFunctions::PCASingle(std::vector<std::vector<double> > &pointData, PCAInfo &pcaInfo)
{
	int i, j;
	int k = pointData.size();
	double a = 1.4826;
	double thRz = 2.5;

	// 
	pcaInfo.idxIn.resize(k);
	Eigen::Vector3d h_mean(0, 0, 0);
	for (i = 0; i < k; ++i)
	{
		pcaInfo.idxIn[i] = i;
		h_mean += Eigen::Vector3d(pointData[i][0], pointData[i][1], pointData[i][2]);
	}
	h_mean *= (1.0 / k);

	Eigen::Matrix3d h_cov = Eigen::Matrix3d::Zero();
	for (i = 0; i < k; ++i)
	{
		Eigen::Vector3d hi(pointData[i][0], pointData[i][1], pointData[i][2]);
		h_cov += (hi - h_mean) * (hi - h_mean).transpose();
	}
	h_cov *= (1.0 / k);

	// eigenvector using Eigen
	Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> eigensolver(h_cov);
	Eigen::Vector3d h_cov_evals = eigensolver.eigenvalues();
	Eigen::Matrix3d h_cov_evectors = eigensolver.eigenvectors();

	// 
	pcaInfo.idxAll = pcaInfo.idxIn;
	//pcaInfo.lambda0 = h_cov_evals(2);
	pcaInfo.lambda0 = h_cov_evals(2) / (h_cov_evals(0) + h_cov_evals(1) + h_cov_evals(2));
	pcaInfo.normal = h_cov_evectors.col(2);
	pcaInfo.planePt = h_mean;

	// outliers removal via MCMD
	MCMD_OutlierRemoval(pointData, pcaInfo);
}

void PCAFunctions::RDPCASingle(std::vector<std::vector<double>>& pointData, PCAInfo & pcaInfo)
{
	int i, j;
	int h0 = 3;
	int k = pointData.size();
	int h = k / 2;
	int iterTime = log(1 - 0.9999) / log(1 - pow(1 - 0.5, h0));
	double a = 1.4826;
	double thRz = 2.5;

	std::vector<std::vector<int> > h_subset_idx_vec;
	for (int iter = 0; iter < iterTime; ++iter)
	{
		int h0_idx0 = rand() % k;
		int h0_idx1 = rand() % k;
		int h0_idx2 = rand() % k;

		Eigen::Vector3d h0_0(pointData[h0_idx0][0], pointData[h0_idx0][1], pointData[h0_idx0][2]);
		Eigen::Vector3d h0_1(pointData[h0_idx1][0], pointData[h0_idx1][1], pointData[h0_idx1][2]);
		Eigen::Vector3d h0_2(pointData[h0_idx2][0], pointData[h0_idx2][1], pointData[h0_idx2][2]);
		Eigen::Vector3d h0_mean = (h0_0 + h0_1 + h0_2) * (1.0 / 3.0);

		// PCA
		Eigen::Matrix3d h0_cov = ((h0_0 - h0_mean) * (h0_0 - h0_mean).transpose()
			+ (h0_1 - h0_mean) * (h0_1 - h0_mean).transpose()
			+ (h0_2 - h0_mean) * (h0_2 - h0_mean).transpose()) * (1.0 / 3.0);

		Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> eigensolver(h0_cov);
		Eigen::Vector3d h0_cov_evals = eigensolver.eigenvalues();
		Eigen::Matrix3d h0_cov_evectors = eigensolver.eigenvectors();

		// OD
		std::vector<std::pair<int, double> > ODs(k);
		for (i = 0; i < k; ++i)
		{
			Eigen::Vector3d ptMat(pointData[i][0], pointData[i][1], pointData[i][2]);
			double OD = fabs((ptMat - h0_mean).dot(h0_cov_evectors.col(2)));
			ODs[i].first = i;
			ODs[i].second = OD;
		}
		std::sort(ODs.begin(), ODs.end(), [](const std::pair<int, double>& lhs, const std::pair<int, double>& rhs) { return lhs.second < rhs.second; });


		// h-subset
		std::vector<int> h_subset_idx;
		for (i = 0; i < h; i++)
		{
			h_subset_idx.push_back(ODs[i].first);
		}
		h_subset_idx_vec.push_back(h_subset_idx);
	}

	// calculate the PCA hypotheses
	std::vector<PCAInfo> S_PCA;
	S_PCA.resize(h_subset_idx_vec.size());
	for (i = 0; i < h_subset_idx_vec.size(); ++i)
	{
		Eigen::Vector3d h_mean(0, 0, 0);
		for (j = 0; j < h; ++j)
		{
			int index = h_subset_idx_vec[i][j];
			h_mean += Eigen::Vector3d(pointData[index][0], pointData[index][1], pointData[index][2]);
		}
		h_mean *= (1.0 / double(h));

		Eigen::Matrix3d h_cov = Eigen::Matrix3d::Zero();
		for (j = 0; j < h; ++j)
		{
			int index = h_subset_idx_vec[i][j];
			Eigen::Vector3d hi(pointData[index][0], pointData[index][1], pointData[index][2]);
			h_cov += (hi - h_mean) * (hi - h_mean).transpose();
		}
		h_cov *= (1.0 / double(h));

		//
		Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> eigensolver(h_cov);
		Eigen::Vector3d h_cov_evals = eigensolver.eigenvalues();
		Eigen::Matrix3d h_cov_evectors = eigensolver.eigenvectors();

		PCAInfo pcaEles;
		//pcaEles.lambda0 = h_cov_evals(2);
		pcaEles.lambda0 = h_cov_evals(2) / (h_cov_evals(0) + h_cov_evals(1) + h_cov_evals(2));
		pcaEles.normal = h_cov_evectors.col(2);
		pcaEles.idxIn = h_subset_idx_vec[i];
		S_PCA[i] = pcaEles;
	}

	// find the best PCA
	std::sort(S_PCA.begin(), S_PCA.end(), [](const PCAInfo& lhs, const PCAInfo& rhs) { return lhs.lambda0 < rhs.lambda0; });
	pcaInfo.lambda0 = S_PCA[0].lambda0;
	pcaInfo.idxIn = S_PCA[0].idxIn;

	pcaInfo.normal = S_PCA[0].normal;
	double N = pcaInfo.normal.norm();
	pcaInfo.normal *= 1.0 / N;

	pcaInfo.idxAll.resize(k);
	for (i = 0; i < k; ++i)
	{
		pcaInfo.idxAll[i] = i;
	}

	// outliers removal via MCMD
	MCMD_OutlierRemoval(pointData, pcaInfo);
}

void PCAFunctions::MCMD_OutlierRemoval(std::vector<std::vector<double> > &pointData, PCAInfo &pcaInfo)
{
	double a = 1.4826;
	double thRz = 2.5;
	int num = pcaInfo.idxAll.size();

	// ODs
	Eigen::Vector3d h_mean(0, 0, 0);
	for (int j = 0; j < pcaInfo.idxIn.size(); ++j)
	{
		int idx = pcaInfo.idxIn[j];
		h_mean += Eigen::Vector3d(pointData[idx][0], pointData[idx][1], pointData[idx][2]);
	}
	h_mean *= (1.0 / pcaInfo.idxIn.size());

	std::vector<double> ODs(num);
	for (int j = 0; j < num; ++j)
	{
		int idx = pcaInfo.idxAll[j];
		Eigen::Vector3d pt(pointData[idx][0], pointData[idx][1], pointData[idx][2]);
		double OD = fabs((pt - h_mean).dot(pcaInfo.normal));
		ODs[j] = OD;
	}

	// calculate the Rz-score for all points using ODs
	std::vector<double> sorted_ODs(ODs.begin(), ODs.end());
	double median_OD = meadian(sorted_ODs);
	std::vector<double>().swap(sorted_ODs);

	std::vector<double> abs_diff_ODs(num);
	for (int j = 0; j < num; ++j)
	{
		abs_diff_ODs[j] = fabs(ODs[j] - median_OD);
	}
	double MAD_OD = a * meadian(abs_diff_ODs) + 1e-6;
	std::vector<double>().swap(abs_diff_ODs);

	// get inlier 
	std::vector<int> idxInlier;
	for (int j = 0; j < num; ++j)
	{
		double Rzi = fabs(ODs[j] - median_OD) / MAD_OD;
		if (Rzi < thRz)
		{
			int idx = pcaInfo.idxAll[j];
			idxInlier.push_back(idx);
		}
	}

	// 
	pcaInfo.idxIn = idxInlier;
}

double PCAFunctions::meadian(std::vector<double> dataset)
{
	std::sort(dataset.begin(), dataset.end(), [](const double& lhs, const double& rhs) { return lhs < rhs; });
	if (dataset.size() % 2 == 0)
	{
		return dataset[dataset.size() / 2];
	}
	else
	{
		return (dataset[dataset.size() / 2] + dataset[dataset.size() / 2 + 1]) / 2.0;
	}
}
