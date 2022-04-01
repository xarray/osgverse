#include "laplacian_deformation.hpp"

#include <Eigen/Sparse>
#include <Eigen/Dense>

#include <math.h>

#include <set>

class vec3 {
public:
	double x, y, z;

	vec3(double x, double y, double z) { this->x = x; this->y = y; this->z = z; }

	vec3(double v) { this->x = v; this->y = v; this->z = v; }

	vec3() { this->x = this->y = this->z = 0; }

	vec3& operator+=(const vec3& b) { (*this) = (*this) + b; return (*this); }
	vec3& operator-=(const vec3& b) { (*this) = (*this) - b; return (*this); }

	friend vec3 operator-(const vec3& a, const vec3& b) { return vec3(a.x - b.x, a.y - b.y, a.z - b.z); }
	friend vec3 operator+(const vec3& a, const vec3& b) { return vec3(a.x + b.x, a.y + b.y, a.z + b.z); }
	friend vec3 operator*(const double s, const vec3& a) { return vec3(s * a.x, s * a.y, s * a.z); }
	friend vec3 operator*(const vec3& a, const double s) { return s * a; }
};


typedef Eigen::SparseMatrix<double> SpMat; 

typedef Eigen::MatrixXd DMat; 

typedef Eigen::Triplet<double> Triplet;

typedef Eigen::VectorXd Vec;

struct Sorter {
	bool operator()(const Triplet& a, const Triplet& b) {
		if (a.row() != b.row()) {
			return a.row() < b.row();
		}
		else {
			return a.col() < b.col();
		}
	}
} sorter;


struct Entry {
	int a0;
	int a1;
	double a2;

	Entry(int a0_, int a1_, double a2_) : a0(a0_), a1(a1_), a2(a2_) {
	}
};

struct SorterEntry {
	bool operator()(const Entry& a, const Entry& b) {
		if (a.a0 != b.a0) {
			return a.a0 < b.a0;
		}
		else {
			return a.a1 < b.a1;
		}
	}
} sorterEntry;

std::vector<Triplet> calcEnergyMatrixCoeffs(
	const Vec& roiPositions,
	const Vec& delta,
	const int nRoi,

	std::vector<std::vector<int> > adj,

	const std::vector<int>& rowBegins,
	const std::vector<Triplet>& laplacianCoeffs
) {
	std::vector<DMat> Ts;

	Ts.resize(nRoi);

	for (int i = 0; i < nRoi; ++i) {
		// set of {i} and the neigbbours of i.
		std::vector<int> iAndNeighbours;

		iAndNeighbours.push_back(i);
		for (int j = 0; j < adj[i].size(); ++j) {
			iAndNeighbours.push_back(adj[i][j]);
		}

		DMat At(7, iAndNeighbours.size() * 3);
		for (int row = 0; row < 7; ++row) {
			for (int col = 0; col < iAndNeighbours.size() * 3; ++col) {
				At(row, col) = 0.0f;
			}
		}

		for (int j = 0; j < iAndNeighbours.size(); ++j) {
			int k = iAndNeighbours[j];

			double vk[3];
			vk[0] = roiPositions[3 * k + 0];
			vk[1] = roiPositions[3 * k + 1];
			vk[2] = roiPositions[3 * k + 2];

			const int x = 0;
			const int y = 1;
			const int z = 2;

			At(0, j * 3 + 0) = +vk[x];
			At(1, j * 3 + 0) = 0;
			At(2, j * 3 + 0) = +vk[z];
			At(3, j * 3 + 0) = -vk[y];
			At(4, j * 3 + 0) = +1;
			At(5, j * 3 + 0) = 0;
			At(6, j * 3 + 0) = 0;

			At(0, j * 3 + 1) = +vk[y];
			At(1, j * 3 + 1) = -vk[z];
			At(2, j * 3 + 1) = 0;
			At(3, j * 3 + 1) = +vk[x];
			At(4, j * 3 + 1) = 0;
			At(5, j * 3 + 1) = +1;
			At(6, j * 3 + 1) = 0;

			At(0, j * 3 + 2) = +vk[z];
			At(1, j * 3 + 2) = +vk[y];
			At(2, j * 3 + 2) = -vk[x];
			At(3, j * 3 + 2) = 0;
			At(4, j * 3 + 2) = 0;
			At(5, j * 3 + 2) = 0;
			At(6, j * 3 + 2) = 1;
		}

		DMat invprod = (At * At.transpose()).inverse();
		DMat pseudoinv = invprod * At;
		Ts[i] = pseudoinv;
		// Ts[i] now contains (A^T A ) A^T (see equation 12 from paper.)
	}

	std::vector<Triplet> result;

	std::map<int, double> row;

	for (int i = 0; i < (nRoi * 3); ++i) {
		row.clear();
		
		// add uniform weights to matrix(equation 2 from paper)
		for (int ientry = rowBegins[i]; ientry < rowBegins[i + 1]; ++ientry) {
			Triplet t = laplacianCoeffs[ientry];
			row[t.col()] = t.value();
		}
	
		// get delta coordinates for the vertex.
		double dx = delta[int(i / 3) * 3 + 0];
		double dy = delta[int(i / 3) * 3 + 1];
		double dz = delta[int(i / 3) * 3 + 2];

		std::vector<int> iAndNeighbours;
		iAndNeighbours.push_back(int(i / 3));
		for (int j = 0; j < adj[int(i / 3)].size(); ++j) {
			iAndNeighbours.push_back(adj[int(i / 3)][j]);
		}

		DMat T = Ts[int(i / 3)];

		Vec s = T.row(0);
		Vec h1 = T.row(1);
		Vec h2 = T.row(2);
		Vec h3 = T.row(3);
		Vec tx = T.row(4);
		Vec ty = T.row(5);
		Vec tz = T.row(6);

		if ((i % 3) == 0) { // x case.
			for (int j = 0; j < T.row(0).size(); ++j) {
				int p = j % 3;
				int q = (int)floor((double)j / (double)3);
				int r = iAndNeighbours[q];

				row[p + 3 * r] -= dx * (+s[j]);
				row[p + 3 * r] -= dy * (-h3[j]);
				row[p + 3 * r] -= dz * (+h2[j]);
			}
		}
		else if ((i % 3) == 1) { // y case.
			for (int j = 0; j < T.row(0).size(); ++j) {
				int p = j % 3;
				int q = (int)floor((double)j / (double)3);
				int r = iAndNeighbours[q];

				row[p + 3 * r] -= dx * (+h3[j]);
				row[p + 3 * r] -= dy * (+s[j]);
				row[p + 3 * r] -= dz * (-h1[j]);
			}
		}
		else if ((i % 3) == 2) { // z case.
			for (int j = 0; j < T.row(0).size(); ++j) {
				int p = j % 3;
				int q = (int)floor((double)j / (double)3);
				int r = iAndNeighbours[q];

				row[p + 3 * r] -= dx * (-h2[j]);
				row[p + 3 * r] -= dy * (+h1[j]);
				row[p + 3 * r] -= dz * (+s[j]);

			}
		}

		for (const auto& p : row) {
			result.push_back(Triplet(i, p.first, p.second));
		}
	}

	return result;
}

double hypot(double x, double y, double z) {
	return sqrt(
		x * x +
		y * y +
		z * z);
}

// cotangent discretization of the laplacian.
std::vector<Triplet> calcCotangentLaplacianCoeffs(

	int* cells, const int nCells,

	const std::vector<int>& roiMap,

	int nRoi,
	std::vector<std::vector<int> > adj,

	std::vector<int>& rowBegins,

	const Vec& roiPositions
) {
	std::vector<Triplet> result;

	std::vector<int> cells_flattened;

	for (int i = 0; i < nCells; i += 3) {
		int c[3] = { roiMap[cells[i + 0]], roiMap[cells[i + 1]] , roiMap[cells[i + 2]] };

		if (c[0] == -1 || c[1] == -1 || c[2] == -1) {
			continue;
		}

		cells_flattened.push_back(c[0]);
		cells_flattened.push_back(c[1]);
		cells_flattened.push_back(c[2]);
	}

	std::map<std::pair<int, int>, double> laplacian;


	std::vector<double> areas;
	for (int i = 0; i < nRoi; ++i) {
		areas.push_back(0);
	}

	std::vector<Entry> entries;
	for (int i = 0; i < cells_flattened.size(); i += 3) {
		int ia = cells_flattened[i + 0];
		int ib = cells_flattened[i + 1];
		int ic = cells_flattened[i + 2];

		vec3 a = vec3(roiPositions[3 * ia + 0], roiPositions[3 * ia + 1], roiPositions[3 * ia + 2]);
		vec3 b = vec3(roiPositions[3 * ib + 0], roiPositions[3 * ib + 1], roiPositions[3 * ib + 2]);
		vec3 c = vec3(roiPositions[3 * ic + 0], roiPositions[3 * ic + 1], roiPositions[3 * ic + 2]);

		double abx = a.x - b.x;
		double aby = a.y - b.y;
		double abz = a.z - b.z;

		double bcx = b.x - c.x;
		double bcy = b.y - c.y;
		double bcz = b.z - c.z;

		double cax = c.x - a.x;
		double cay = c.y - a.y;
		double caz = c.z - a.z;

		double area = 0.5 * hypot(
			aby * caz - abz * cay,
			abz * cax - abx * caz,
			abx * cay - aby * cax);

		//Skip thin triangles
		if (area < 1e-8) {
			continue;
		}

		double w = -0.5 / area;
		double wa = w * (abx * cax + aby * cay + abz * caz);
		double wb = w * (bcx * abx + bcy * aby + bcz * abz);
		double wc = w * (cax * bcx + cay * bcy + caz * bcz);

		double varea = area / 3.0;
		areas[ia] += varea;
		areas[ib] += varea;
		areas[ic] += varea;

		entries.push_back(Entry(ib, ic, wa));
		entries.push_back(Entry(ic, ib, wa));
		entries.push_back(Entry(ic, ia, wb));
		entries.push_back(Entry(ia, ic, wb));
		entries.push_back(Entry(ia, ib, wc));
		entries.push_back(Entry(ib, ia, wc));
	}

	std::vector<double> weights;
	for (int i = 0; i < nRoi; ++i) {
		weights.push_back(0.0);
	}

	std::sort(entries.begin(), entries.end(), sorterEntry);

	int ptr = 0;

	for (int i = 0; i < entries.size(); ) {

		Entry entry = entries[i++];

		while (
			i < entries.size() &&
			entries[i].a0 == entry.a0 &&
			entries[i].a1 == entry.a1) {
			entry.a2 += entries[i++].a2;

		}

		entry.a2 /= areas[entry.a0];
		weights[entry.a0] += entry.a2;
		entries[ptr++] = entry;
	}

	for (int i = 0; i < ptr; ++i) {
		std::pair<int, int> e(entries[i].a0, entries[i].a1);
		Triplet t(e.first, e.second, entries[i].a2);
		result.push_back(t);
	}

	for (int i = 0; i < nRoi; ++i) {
		Triplet t(i, i, -weights[i]);
		result.push_back(t);
	}
	std::sort(result.begin(), result.end(), sorter);

	{
		std::vector<Triplet> result2;

		for (int i = 0; i < result.size(); ++i) {

			Triplet a = result[i];

			result2.push_back(Triplet(3 * a.row() + 0, 3 * a.col() + 0, a.value()));

			result2.push_back(Triplet(3 * a.row() + 1, 3 * a.col() + 1, a.value()));

			result2.push_back(Triplet(3 * a.row() + 2, 3 * a.col() + 2, a.value()));

		}

		result = result2;

		std::sort(result.begin(), result.end(), sorter);
	}

	int current = result[0].row();
	rowBegins.push_back(0);
	for (int i = 0; i < result.size(); ++i) {

		if (result[i].row() != current) {
			rowBegins.push_back(i);
			current = result[i].row();
		}
	}
	rowBegins.push_back(result.size());

	return result;
}

std::vector<Triplet> calcUniformLaplacianCoeffs(
	int nRoi,
	std::vector<std::vector<int> > adj,

	std::vector<int>& rowBegins
) {
	std::vector<Triplet> result;
	std::map<int, double> row;

	for (int i = 0; i < (nRoi * 3); ++i) {
		rowBegins.push_back(result.size());
		row.clear();

		row[(i % 3) + int(i / 3) * 3] = 1;
		double w = -1.0 / adj[int(i / 3)].size();
		for (int j = 0; j < adj[int(i / 3)].size(); ++j) {
			row[(i % 3) + 3 * adj[int(i / 3)][j]] = w;
		}

		for (const auto& p : row) {
			result.push_back(Triplet(i, p.first, p.second));
		}
	}
	rowBegins.push_back(result.size());

	return result;
}

struct State {
	bool RSI;

	Vec roiDelta;

	SpMat augEnergyMatrixTrans;
	SpMat augNormalizeDeltaCoordinatesTrans;

	Eigen::SimplicialCholesky<SpMat>*energyMatrixCholesky = nullptr;
	Eigen::SimplicialCholesky<SpMat>*normalizeDeltaCoordinatesCholesky = nullptr;

	int* roiIndices;
	int nRoi;

	SpMat lapMat;

	std::vector<double> roiDeltaLengths;

	Vec b;
};

State s;

double getLength(double ax, double ay, double az) {
	return sqrt(ax*ax + ay * ay + az * az);
}

void freeDeform() {
	if (s.energyMatrixCholesky != nullptr) {
		delete s.energyMatrixCholesky;
		s.energyMatrixCholesky = nullptr;
	}
	if (s.normalizeDeltaCoordinatesCholesky != nullptr) {
		delete s.normalizeDeltaCoordinatesCholesky;
		s.normalizeDeltaCoordinatesCholesky = nullptr;
	}
}

/*
For reference, the equation numbers refer to the paper:
https://people.eecs.berkeley.edu/~jrs/meshpapers/SCOLARS.pdf
*/
void prepareDeform(
	int* cells, const int nCells,

	double* positions, const int nPositions,

	int* roiIndices, const int nRoi,

	const int unconstrainedBegin,

	bool RSI) {

	// free memory from previous call of prepareDeform()
	if (s.energyMatrixCholesky != nullptr) {
		delete s.energyMatrixCholesky;
		s.energyMatrixCholesky = nullptr;
	}
	if (s.normalizeDeltaCoordinatesCholesky != nullptr) {
		delete s.normalizeDeltaCoordinatesCholesky;
		s.normalizeDeltaCoordinatesCholesky = nullptr;
	}

	std::vector<std::vector<int> > adj;
	std::vector<int> roiMap(nPositions, -1);

	{
		for (int i = 0; i < nRoi; ++i) {
			roiMap[roiIndices[i]] = i;
		}

		adj.resize(nRoi);
		for (int i = 0; i < adj.size(); ++i) {
			adj[i] = std::vector<int>();
		}
		for (int i = 0; i < nCells; i += 3) {
			int c[3] = { cells[i + 0], cells[i + 1] , cells[i + 2] };

			for (int j = 0; j < 3; ++j) {
				int a = roiMap[c[j]];

				int b = roiMap[c[(j + 1) % 3]];

				if (a != -1 && b != -1) {
					adj[a].push_back(b);
				}
			}
		}
	}

	// put all the positions of the vertices in ROI in a single vector.
	Vec roiPositions(nRoi * 3);
	{
		int c = 0;
		for (int i = 0; i < nRoi; ++i) {
			for (int d = 0; d < 3; ++d) {
				roiPositions[c++] = positions[3 * roiIndices[i] + d];
			}
		}
	}

	std::vector<int> rowBegins;
	std::vector<Triplet> laplacianCoeffs;
	
	/*
	// cotangent laplacian doesnt yield any good results, for some reason :/
	so we don't use it. instead, use uniform.
	laplacianCoeffs = calcCotangentLaplacianCoeffs(
		
		cells, nCells, 
		roiMap,

		nRoi,
		adj,
		rowBegins,
	
		roiPositions);
		*/	
	  
	laplacianCoeffs = calcUniformLaplacianCoeffs(nRoi, adj, rowBegins);

	s.lapMat = SpMat(nRoi * 3, nRoi * 3);
	s.lapMat.setFromTriplets(laplacianCoeffs.begin(), laplacianCoeffs.end());

	// by simply multiplying by the laplacian matrix, we can compute the laplacian coordinates(the delta coordinates)
	// of the vertices in ROI.
	s.roiDelta = s.lapMat * roiPositions;

	// we save away the original lengths of the delta coordinates.
	// we need these when normalizing the results of our solver.
	{
		s.roiDeltaLengths = std::vector<double>(s.roiDelta.size() / 3, 0.0f);
		for (int i = 0; i < s.roiDelta.size() / 3; ++i) {
			s.roiDeltaLengths[i] = getLength(
				s.roiDelta[3 * i + 0],
				s.roiDelta[3 * i + 1],
				s.roiDelta[3 * i + 2]
			);
		}
	}

	std::vector<Triplet> energyMatrixCoeffs;

	// num rows in augmented matrix. 
	// notice that we put x, y, and z in a large single matrix, and therefore it is multiplied by 3.
	int M = (nRoi + unconstrainedBegin) * 3;
	// num columns in augmented matrix.
	int N = nRoi * 3;

	if (RSI) {
		// this matrix represents the first term of the energy (5).
		energyMatrixCoeffs = calcEnergyMatrixCoeffs(
			roiPositions,
			s.roiDelta, nRoi, adj, rowBegins, laplacianCoeffs);

		for (int i = 0; i < unconstrainedBegin; ++i) {
			laplacianCoeffs.push_back(Triplet(i * 3 + N + 0, 3 * i + 0, 1));
			laplacianCoeffs.push_back(Triplet(i * 3 + N + 1, 3 * i + 1, 1));
			laplacianCoeffs.push_back(Triplet(i * 3 + N + 2, 3 * i + 2, 1));
		}

		SpMat augMat(M, N);
		augMat.setFromTriplets(laplacianCoeffs.begin(), laplacianCoeffs.end());
		s.augNormalizeDeltaCoordinatesTrans = augMat.transpose();

		s.normalizeDeltaCoordinatesCholesky = new Eigen::SimplicialCholesky<SpMat>(s.augNormalizeDeltaCoordinatesTrans * augMat);
	}
	else {
		// if not rotation-scale-invariant, we simply use the regular laplacian matrix. This is the first term of the energy (4)
		energyMatrixCoeffs = laplacianCoeffs;
	}

	// in order to add the second term of the energy (4) or (5), we now augment the matrix.
	{
		// we augment the matrix by adding constraints for the handles.
		// these constraints ensure that if the handles are dragged, the handles will strictly follow in the specified direction.
		// the handle vertices are not free, unlike the unconstrained vertices.
		for (int i = 0; i < unconstrainedBegin; ++i) {
			energyMatrixCoeffs.push_back(Triplet(i * 3 + N + 0, 3 * i + 0, 1));
			energyMatrixCoeffs.push_back(Triplet(i * 3 + N + 1, 3 * i + 1, 1));
			energyMatrixCoeffs.push_back(Triplet(i * 3 + N + 2, 3 * i + 2, 1));
		}

		SpMat augMat(M, N);
		augMat.setFromTriplets(energyMatrixCoeffs.begin(), energyMatrixCoeffs.end());
		s.augEnergyMatrixTrans = augMat.transpose();

		// for solving later, we need the cholesky decomposition of (transpose(augMat) * augMat)
		// this is a slow step! probably the slowest part of the entire algorithm.
		s.energyMatrixCholesky = new Eigen::SimplicialCholesky<SpMat>(s.augEnergyMatrixTrans * augMat);
	}

	s.b = Vec(M);
	s.roiIndices = roiIndices;
	s.nRoi = nRoi;
	s.RSI = RSI;
}

void doDeform(double* newHandlePositions, int nHandlePositions, double* outPositions) {
	{
		int count = 0;
		for (int i = 0; i < s.roiDelta.size(); ++i) {
			if (s.RSI) {
				// following from our derivations, we must set all these to zero. 
				s.b[count++] = 0.0f;
			}
			else {
				s.b[count++] = s.roiDelta[i];
			}
		}
		for (int j = 0; j < nHandlePositions; ++j) {
			s.b[count++] = newHandlePositions[j * 3 + 0];
			s.b[count++] = newHandlePositions[j * 3 + 1];
			s.b[count++] = newHandlePositions[j * 3 + 2];
		}
	}

	Vec minimizerSolution;
	{
		// Now we solve 
		// Ax = b
		// where A is the energy matrix, and the value of b depends on whether we are optimizing (4) or (5)
		// by solving, we obtain the deformed surface coordinates that minimizes either (4) or (5).
		Vec y = s.augEnergyMatrixTrans * s.b;
		minimizerSolution = s.energyMatrixCholesky->solve(y);
	}

	if (s.RSI) {
		// if minimizing (5), a local scaling is introduced by the solver.
		// so we need to normalize the delta coordinates of the deformed vertices back to their
		// original lengths.
		// otherwise, the mesh will increase in size when manipulating the mesh, which is not desirable.

		// the normalization step is pretty simple:
		// we find the delta coordinates of our solution.			
		// then we normalize these delta coordinates, so that their lengths match the lengths of the original, undeformed delta coordinates.			
		// then we simply do a minimization to find the coordinates that are as close as possible to the normalized delta coordinates	
		// and the solution of this minimization is our final solution.

		Vec solutionDelta = s.lapMat * minimizerSolution;

		int count = 0;
		for (int i = 0; i < s.roiDeltaLengths.size(); ++i) {

			double len = getLength(solutionDelta[3 * i + 0], solutionDelta[3 * i + 1], solutionDelta[3 * i + 2]);
			double originalLength = s.roiDeltaLengths[i];
			double scale = originalLength / len;

			for (int d = 0; d < 3; ++d) {
				s.b[count++] = scale * solutionDelta[3 * i + d];
			}
		}

		Vec y = s.augNormalizeDeltaCoordinatesTrans * s.b;
		Vec normalizedSolution = s.normalizeDeltaCoordinatesCholesky->solve(y);

		for (int i = 0; i < s.nRoi; ++i) {
			for (int d = 0; d < 3; ++d) {
				outPositions[3 * s.roiIndices[i] + d] = normalizedSolution[3 * i + d];
			}
		}
	}
	else {
		for (int i = 0; i < s.nRoi; ++i) {
			for (int d = 0; d < 3; ++d) {
				outPositions[3 * s.roiIndices[i] + d] = minimizerSolution[3 * i + d];
			}
		}
	}
}

