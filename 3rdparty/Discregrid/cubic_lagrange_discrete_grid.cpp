#include "data/z_sort_table.hpp"
#include "cubic_lagrange_discrete_grid.hpp"
#include <utility/serialize.hpp>
#include "utility/spinlock.hpp"
#include "utility/timing.hpp"

#include <iostream>
#include <iomanip>
#include <atomic>
#include <numeric>
#include <set>
#include <chrono>
#include <future>

using namespace Eigen;

namespace Discregrid
{

namespace
{

double const abscissae[32][3] = {
	{-1.000000000000, -1.000000000000, -1.000000000000}, // 0
	{-1.000000000000, -1.000000000000, 1.000000000000},  // 1
	{-1.000000000000, 1.000000000000, -1.000000000000},  // 2
	{-1.000000000000, 1.000000000000, 1.000000000000},   // 3
	{1.000000000000, -1.000000000000, -1.000000000000},  // 4
	{1.000000000000, -1.000000000000, 1.000000000000},   // 5
	{1.000000000000, 1.000000000000, -1.000000000000},   // 6
	{1.000000000000, 1.000000000000, 1.000000000000},	// 7
	{-0.333333333333, -1.000000000000, -1.000000000000}, // 8
	{-0.333333333333, -1.000000000000, 1.000000000000},  // 9
	{-0.333333333333, 1.000000000000, -1.000000000000},  //10
	{-0.333333333333, 1.000000000000, 1.000000000000},   //11
	{0.333333333333, -1.000000000000, -1.000000000000},  //12
	{0.333333333333, -1.000000000000, 1.000000000000},   //13
	{0.333333333333, 1.000000000000, -1.000000000000},   //14
	{0.333333333333, 1.000000000000, 1.000000000000},	//15
	{-1.000000000000, -0.333333333333, -1.000000000000}, //16
	{-1.000000000000, -0.333333333333, 1.000000000000},  //17
	{-1.000000000000, 0.333333333333, -1.000000000000},  //18
	{-1.000000000000, 0.333333333333, 1.000000000000},   //19
	{1.000000000000, -0.333333333333, -1.000000000000},  //20
	{1.000000000000, -0.333333333333, 1.000000000000},   //21
	{1.000000000000, 0.333333333333, -1.000000000000},   //22
	{1.000000000000, 0.333333333333, 1.000000000000},	//23
	{-1.000000000000, -1.000000000000, -0.333333333333}, //24
	{-1.000000000000, -1.000000000000, 0.333333333333},  //25
	{-1.000000000000, 1.000000000000, -0.333333333333},  //26
	{-1.000000000000, 1.000000000000, 0.333333333333},   //27
	{1.000000000000, -1.000000000000, -0.333333333333},  //28
	{1.000000000000, -1.000000000000, 0.333333333333},   //29
	{1.000000000000, 1.000000000000, -0.333333333333},   //30
	{1.000000000000, 1.000000000000, 0.333333333333}	 //31
};

double const abscissae_[32][3] = {
	{-1.000000000000, -1.000000000000, -1.000000000000}, // 0 -->  0
	{1.000000000000, -1.000000000000, -1.000000000000},  // 4 -->  1
	{-1.000000000000, 1.000000000000, -1.000000000000},  // 2 -->  2
	{1.000000000000, 1.000000000000, -1.000000000000},   // 6 -->  3
	{-1.000000000000, -1.000000000000, 1.000000000000},  // 1 -->  4
	{1.000000000000, -1.000000000000, 1.000000000000},   // 5 -->  5
	{-1.000000000000, 1.000000000000, 1.000000000000},   // 3 -->  6
	{1.000000000000, 1.000000000000, 1.000000000000},	// 7 -->  7

	{-0.333333333333, -1.000000000000, -1.000000000000}, // 8 -->  8
	{0.333333333333, -1.000000000000, -1.000000000000},  //12 -->  9
	{-0.333333333333, -1.000000000000, 1.000000000000},  // 9 --> 10
	{0.333333333333, -1.000000000000, 1.000000000000},   //13 --> 11
	{-0.333333333333, 1.000000000000, -1.000000000000},  //10 --> 12
	{0.333333333333, 1.000000000000, -1.000000000000},   //14 --> 13
	{-0.333333333333, 1.000000000000, 1.000000000000},   //11 --> 14
	{0.333333333333, 1.000000000000, 1.000000000000},	//15 --> 15

	{-1.000000000000, -0.333333333333, -1.000000000000}, //16 --> 16
	{-1.000000000000, 0.333333333333, -1.000000000000},  //18 --> 17
	{1.000000000000, -0.333333333333, -1.000000000000},  //20 --> 18
	{1.000000000000, 0.333333333333, -1.000000000000},   //22 --> 19
	{-1.000000000000, -0.333333333333, 1.000000000000},  //17 --> 20
	{-1.000000000000, 0.333333333333, 1.000000000000},   //19 --> 21
	{1.000000000000, -0.333333333333, 1.000000000000},   //21 --> 22
	{1.000000000000, 0.333333333333, 1.000000000000},	//23 --> 23

	{-1.000000000000, -1.000000000000, -0.333333333333}, //24 --> 24
	{-1.000000000000, -1.000000000000, 0.333333333333},  //25 --> 25
	{-1.000000000000, 1.000000000000, -0.333333333333},  //26 --> 26
	{-1.000000000000, 1.000000000000, 0.333333333333},   //27 --> 27
	{1.000000000000, -1.000000000000, -0.333333333333},  //28 --> 28
	{1.000000000000, -1.000000000000, 0.333333333333},   //29 --> 29
	{1.000000000000, 1.000000000000, -0.333333333333},   //30 --> 30
	{1.000000000000, 1.000000000000, 0.333333333333}	 //31 --> 31
};

Matrix<double, 32, 1>
shape_function(Vector3d const &xi, Matrix<double, 32, 3> *gradient = nullptr)
{
	auto res = Matrix<double, 32, 1>{};

	auto x = xi[0];
	auto y = xi[1];
	auto z = xi[2];

	auto x2 = x * x;
	auto y2 = y * y;
	auto z2 = z * z;

	auto _1mx = 1.0 - x;
	auto _1my = 1.0 - y;
	auto _1mz = 1.0 - z;

	auto _1px = 1.0 + x;
	auto _1py = 1.0 + y;
	auto _1pz = 1.0 + z;

	auto _1m3x = 1.0 - 3.0 * x;
	auto _1m3y = 1.0 - 3.0 * y;
	auto _1m3z = 1.0 - 3.0 * z;

	auto _1p3x = 1.0 + 3.0 * x;
	auto _1p3y = 1.0 + 3.0 * y;
	auto _1p3z = 1.0 + 3.0 * z;

	auto _1mxt1my = _1mx * _1my;
	auto _1mxt1py = _1mx * _1py;
	auto _1pxt1my = _1px * _1my;
	auto _1pxt1py = _1px * _1py;

	auto _1mxt1mz = _1mx * _1mz;
	auto _1mxt1pz = _1mx * _1pz;
	auto _1pxt1mz = _1px * _1mz;
	auto _1pxt1pz = _1px * _1pz;

	auto _1myt1mz = _1my * _1mz;
	auto _1myt1pz = _1my * _1pz;
	auto _1pyt1mz = _1py * _1mz;
	auto _1pyt1pz = _1py * _1pz;

	auto _1mx2 = 1.0 - x2;
	auto _1my2 = 1.0 - y2;
	auto _1mz2 = 1.0 - z2;

	// Corner nodes.
	auto fac = 1.0 / 64.0 * (9.0 * (x2 + y2 + z2) - 19.0);
	res[0] = fac * _1mxt1my * _1mz;
	res[1] = fac * _1mxt1my * _1pz;
	res[2] = fac * _1mxt1py * _1mz;
	res[3] = fac * _1mxt1py * _1pz;
	res[4] = fac * _1pxt1my * _1mz;
	res[5] = fac * _1pxt1my * _1pz;
	res[6] = fac * _1pxt1py * _1mz;
	res[7] = fac * _1pxt1py * _1pz;

	// Edge nodes.

	fac = 9.0 / 64.0 * _1mx2;
	auto fact1m3x = fac * _1m3x;
	auto fact1p3x = fac * _1p3x;
	res[8] = fact1m3x * _1myt1mz;
	res[9] = fact1m3x * _1myt1pz;
	res[10] = fact1m3x * _1pyt1mz;
	res[11] = fact1m3x * _1pyt1pz;
	res[12] = fact1p3x * _1myt1mz;
	res[13] = fact1p3x * _1myt1pz;
	res[14] = fact1p3x * _1pyt1mz;
	res[15] = fact1p3x * _1pyt1pz;

	fac = 9.0 / 64.0 * _1my2;
	auto fact1m3y = fac * _1m3y;
	auto fact1p3y = fac * _1p3y;
	res[16] = fact1m3y * _1mxt1mz;
	res[17] = fact1m3y * _1mxt1pz;
	res[18] = fact1p3y * _1mxt1mz;
	res[19] = fact1p3y * _1mxt1pz;
	res[20] = fact1m3y * _1pxt1mz;
	res[21] = fact1m3y * _1pxt1pz;
	res[22] = fact1p3y * _1pxt1mz;
	res[23] = fact1p3y * _1pxt1pz;

	fac = 9.0 / 64.0 * _1mz2;
	auto fact1m3z = fac * _1m3z;
	auto fact1p3z = fac * _1p3z;
	res[24] = fact1m3z * _1mxt1my;
	res[25] = fact1p3z * _1mxt1my;
	res[26] = fact1m3z * _1mxt1py;
	res[27] = fact1p3z * _1mxt1py;
	res[28] = fact1m3z * _1pxt1my;
	res[29] = fact1p3z * _1pxt1my;
	res[30] = fact1m3z * _1pxt1py;
	res[31] = fact1p3z * _1pxt1py;

	if (gradient)
	{
		auto &dN = *gradient;

		auto _9t3x2py2pz2m19 = 9.0 * (3.0 * x2 + y2 + z2) - 19.0;
		auto _9tx2p3y2pz2m19 = 9.0 * (x2 + 3.0 * y2 + z2) - 19.0;
		auto _9tx2py2p3z2m19 = 9.0 * (x2 + y2 + 3.0 * z2) - 19.0;
		auto _18x = 18.0 * x;
		auto _18y = 18.0 * y;
		auto _18z = 18.0 * z;

		auto _3m9x2 = 3.0 - 9.0 * x2;
		auto _3m9y2 = 3.0 - 9.0 * y2;
		auto _3m9z2 = 3.0 - 9.0 * z2;

		auto _2x = 2.0 * x;
		auto _2y = 2.0 * y;
		auto _2z = 2.0 * z;

		auto _18xm9t3x2py2pz2m19 = _18x - _9t3x2py2pz2m19;
		auto _18xp9t3x2py2pz2m19 = _18x + _9t3x2py2pz2m19;
		auto _18ym9tx2p3y2pz2m19 = _18y - _9tx2p3y2pz2m19;
		auto _18yp9tx2p3y2pz2m19 = _18y + _9tx2p3y2pz2m19;
		auto _18zm9tx2py2p3z2m19 = _18z - _9tx2py2p3z2m19;
		auto _18zp9tx2py2p3z2m19 = _18z + _9tx2py2p3z2m19;

		dN(0, 0) = _18xm9t3x2py2pz2m19 * _1myt1mz;
		dN(0, 1) = _1mxt1mz * _18ym9tx2p3y2pz2m19;
		dN(0, 2) = _1mxt1my * _18zm9tx2py2p3z2m19;
		dN(1, 0) = _18xm9t3x2py2pz2m19 * _1myt1pz;
		dN(1, 1) = _1mxt1pz * _18ym9tx2p3y2pz2m19;
		dN(1, 2) = _1mxt1my * _18zp9tx2py2p3z2m19;
		dN(2, 0) = _18xm9t3x2py2pz2m19 * _1pyt1mz;
		dN(2, 1) = _1mxt1mz * _18yp9tx2p3y2pz2m19;
		dN(2, 2) = _1mxt1py * _18zm9tx2py2p3z2m19;
		dN(3, 0) = _18xm9t3x2py2pz2m19 * _1pyt1pz;
		dN(3, 1) = _1mxt1pz * _18yp9tx2p3y2pz2m19;
		dN(3, 2) = _1mxt1py * _18zp9tx2py2p3z2m19;
		dN(4, 0) = _18xp9t3x2py2pz2m19 * _1myt1mz;
		dN(4, 1) = _1pxt1mz * _18ym9tx2p3y2pz2m19;
		dN(4, 2) = _1pxt1my * _18zm9tx2py2p3z2m19;
		dN(5, 0) = _18xp9t3x2py2pz2m19 * _1myt1pz;
		dN(5, 1) = _1pxt1pz * _18ym9tx2p3y2pz2m19;
		dN(5, 2) = _1pxt1my * _18zp9tx2py2p3z2m19;
		dN(6, 0) = _18xp9t3x2py2pz2m19 * _1pyt1mz;
		dN(6, 1) = _1pxt1mz * _18yp9tx2p3y2pz2m19;
		dN(6, 2) = _1pxt1py * _18zm9tx2py2p3z2m19;
		dN(7, 0) = _18xp9t3x2py2pz2m19 * _1pyt1pz;
		dN(7, 1) = _1pxt1pz * _18yp9tx2p3y2pz2m19;
		dN(7, 2) = _1pxt1py * _18zp9tx2py2p3z2m19;

		dN.topRows(8) /= 64.0;

		auto _m3m9x2m2x = -_3m9x2 - _2x;
		auto _p3m9x2m2x = _3m9x2 - _2x;
		auto _1mx2t1m3x = _1mx2 * _1m3x;
		auto _1mx2t1p3x = _1mx2 * _1p3x;
		dN(8, 0) = _m3m9x2m2x * _1myt1mz,
			  dN(8, 1) = -_1mx2t1m3x * _1mz,
			  dN(8, 2) = -_1mx2t1m3x * _1my;
		dN(9, 0) = _m3m9x2m2x * _1myt1pz,
			  dN(9, 1) = -_1mx2t1m3x * _1pz,
			  dN(9, 2) = _1mx2t1m3x * _1my;
		dN(10, 0) = _m3m9x2m2x * _1pyt1mz,
			   dN(10, 1) = _1mx2t1m3x * _1mz,
			   dN(10, 2) = -_1mx2t1m3x * _1py;
		dN(11, 0) = _m3m9x2m2x * _1pyt1pz,
			   dN(11, 1) = _1mx2t1m3x * _1pz,
			   dN(11, 2) = _1mx2t1m3x * _1py;
		dN(12, 0) = _p3m9x2m2x * _1myt1mz,
			   dN(12, 1) = -_1mx2t1p3x * _1mz,
			   dN(12, 2) = -_1mx2t1p3x * _1my;
		dN(13, 0) = _p3m9x2m2x * _1myt1pz,
			   dN(13, 1) = -_1mx2t1p3x * _1pz,
			   dN(13, 2) = _1mx2t1p3x * _1my;
		dN(14, 0) = _p3m9x2m2x * _1pyt1mz,
			   dN(14, 1) = _1mx2t1p3x * _1mz,
			   dN(14, 2) = -_1mx2t1p3x * _1py;
		dN(15, 0) = _p3m9x2m2x * _1pyt1pz,
			   dN(15, 1) = _1mx2t1p3x * _1pz,
			   dN(15, 2) = _1mx2t1p3x * _1py;

		auto _m3m9y2m2y = -_3m9y2 - _2y;
		auto _p3m9y2m2y = _3m9y2 - _2y;
		auto _1my2t1m3y = _1my2 * _1m3y;
		auto _1my2t1p3y = _1my2 * _1p3y;
		dN(16, 0) = -_1my2t1m3y * _1mz,
			   dN(16, 1) = _m3m9y2m2y * _1mxt1mz,
			   dN(16, 2) = -_1my2t1m3y * _1mx;
		dN(17, 0) = -_1my2t1m3y * _1pz,
			   dN(17, 1) = _m3m9y2m2y * _1mxt1pz,
			   dN(17, 2) = _1my2t1m3y * _1mx;
		dN(18, 0) = -_1my2t1p3y * _1mz,
			   dN(18, 1) = _p3m9y2m2y * _1mxt1mz,
			   dN(18, 2) = -_1my2t1p3y * _1mx;
		dN(19, 0) = -_1my2t1p3y * _1pz,
			   dN(19, 1) = _p3m9y2m2y * _1mxt1pz,
			   dN(19, 2) = _1my2t1p3y * _1mx;
		dN(20, 0) = _1my2t1m3y * _1mz,
			   dN(20, 1) = _m3m9y2m2y * _1pxt1mz,
			   dN(20, 2) = -_1my2t1m3y * _1px;
		dN(21, 0) = _1my2t1m3y * _1pz,
			   dN(21, 1) = _m3m9y2m2y * _1pxt1pz,
			   dN(21, 2) = _1my2t1m3y * _1px;
		dN(22, 0) = _1my2t1p3y * _1mz,
			   dN(22, 1) = _p3m9y2m2y * _1pxt1mz,
			   dN(22, 2) = -_1my2t1p3y * _1px;
		dN(23, 0) = _1my2t1p3y * _1pz,
			   dN(23, 1) = _p3m9y2m2y * _1pxt1pz,
			   dN(23, 2) = _1my2t1p3y * _1px;

		auto _m3m9z2m2z = -_3m9z2 - _2z;
		auto _p3m9z2m2z = _3m9z2 - _2z;
		auto _1mz2t1m3z = _1mz2 * _1m3z;
		auto _1mz2t1p3z = _1mz2 * _1p3z;
		dN(24, 0) = -_1mz2t1m3z * _1my,
			   dN(24, 1) = -_1mz2t1m3z * _1mx,
			   dN(24, 2) = _m3m9z2m2z * _1mxt1my;
		dN(25, 0) = -_1mz2t1p3z * _1my,
			   dN(25, 1) = -_1mz2t1p3z * _1mx,
			   dN(25, 2) = _p3m9z2m2z * _1mxt1my;
		dN(26, 0) = -_1mz2t1m3z * _1py,
			   dN(26, 1) = _1mz2t1m3z * _1mx,
			   dN(26, 2) = _m3m9z2m2z * _1mxt1py;
		dN(27, 0) = -_1mz2t1p3z * _1py,
			   dN(27, 1) = _1mz2t1p3z * _1mx,
			   dN(27, 2) = _p3m9z2m2z * _1mxt1py;
		dN(28, 0) = _1mz2t1m3z * _1my,
			   dN(28, 1) = -_1mz2t1m3z * _1px,
			   dN(28, 2) = _m3m9z2m2z * _1pxt1my;
		dN(29, 0) = _1mz2t1p3z * _1my,
			   dN(29, 1) = -_1mz2t1p3z * _1px,
			   dN(29, 2) = _p3m9z2m2z * _1pxt1my;
		dN(30, 0) = _1mz2t1m3z * _1py,
			   dN(30, 1) = _1mz2t1m3z * _1px,
			   dN(30, 2) = _m3m9z2m2z * _1pxt1py;
		dN(31, 0) = _1mz2t1p3z * _1py,
			   dN(31, 1) = _1mz2t1p3z * _1px,
			   dN(31, 2) = _p3m9z2m2z * _1pxt1py;

		dN.bottomRows(32u - 8u) *= 9.0 / 64.0;
	}

	return res;
}

Matrix<double, 32, 1>
shape_function_(Vector3d const &xi, Matrix<double, 32, 3> *gradient = nullptr)
{
	auto res = Matrix<double, 32, 1>{};

	auto x = xi[0];
	auto y = xi[1];
	auto z = xi[2];

	auto x2 = x * x;
	auto y2 = y * y;
	auto z2 = z * z;

	auto _1mx = 1.0 - x;
	auto _1my = 1.0 - y;
	auto _1mz = 1.0 - z;

	auto _1px = 1.0 + x;
	auto _1py = 1.0 + y;
	auto _1pz = 1.0 + z;

	auto _1m3x = 1.0 - 3.0 * x;
	auto _1m3y = 1.0 - 3.0 * y;
	auto _1m3z = 1.0 - 3.0 * z;

	auto _1p3x = 1.0 + 3.0 * x;
	auto _1p3y = 1.0 + 3.0 * y;
	auto _1p3z = 1.0 + 3.0 * z;

	auto _1mxt1my = _1mx * _1my;
	auto _1mxt1py = _1mx * _1py;
	auto _1pxt1my = _1px * _1my;
	auto _1pxt1py = _1px * _1py;

	auto _1mxt1mz = _1mx * _1mz;
	auto _1mxt1pz = _1mx * _1pz;
	auto _1pxt1mz = _1px * _1mz;
	auto _1pxt1pz = _1px * _1pz;

	auto _1myt1mz = _1my * _1mz;
	auto _1myt1pz = _1my * _1pz;
	auto _1pyt1mz = _1py * _1mz;
	auto _1pyt1pz = _1py * _1pz;

	auto _1mx2 = 1.0 - x2;
	auto _1my2 = 1.0 - y2;
	auto _1mz2 = 1.0 - z2;

	// Corner nodes.
	auto fac = 1.0 / 64.0 * (9.0 * (x2 + y2 + z2) - 19.0);
	res[0] = fac * _1mxt1my * _1mz;
	res[1] = fac * _1pxt1my * _1mz;
	res[2] = fac * _1mxt1py * _1mz;
	res[3] = fac * _1pxt1py * _1mz;
	res[4] = fac * _1mxt1my * _1pz;
	res[5] = fac * _1pxt1my * _1pz;
	res[6] = fac * _1mxt1py * _1pz;
	res[7] = fac * _1pxt1py * _1pz;

	// Edge nodes.

	fac = 9.0 / 64.0 * _1mx2;
	auto fact1m3x = fac * _1m3x;
	auto fact1p3x = fac * _1p3x;
	res[8] = fact1m3x * _1myt1mz;
	res[9] = fact1p3x * _1myt1mz;
	res[10] = fact1m3x * _1myt1pz;
	res[11] = fact1p3x * _1myt1pz;
	res[12] = fact1m3x * _1pyt1mz;
	res[13] = fact1p3x * _1pyt1mz;
	res[14] = fact1m3x * _1pyt1pz;
	res[15] = fact1p3x * _1pyt1pz;

	fac = 9.0 / 64.0 * _1my2;
	auto fact1m3y = fac * _1m3y;
	auto fact1p3y = fac * _1p3y;
	res[16] = fact1m3y * _1mxt1mz;
	res[17] = fact1p3y * _1mxt1mz;
	res[18] = fact1m3y * _1pxt1mz;
	res[19] = fact1p3y * _1pxt1mz;
	res[20] = fact1m3y * _1mxt1pz;
	res[21] = fact1p3y * _1mxt1pz;
	res[22] = fact1m3y * _1pxt1pz;
	res[23] = fact1p3y * _1pxt1pz;

	fac = 9.0 / 64.0 * _1mz2;
	auto fact1m3z = fac * _1m3z;
	auto fact1p3z = fac * _1p3z;
	res[24] = fact1m3z * _1mxt1my;
	res[25] = fact1p3z * _1mxt1my;
	res[26] = fact1m3z * _1mxt1py;
	res[27] = fact1p3z * _1mxt1py;
	res[28] = fact1m3z * _1pxt1my;
	res[29] = fact1p3z * _1pxt1my;
	res[30] = fact1m3z * _1pxt1py;
	res[31] = fact1p3z * _1pxt1py;

	if (gradient)
	{
		auto &dN = *gradient;

		auto _9t3x2py2pz2m19 = 9.0 * (3.0 * x2 + y2 + z2) - 19.0;
		auto _9tx2p3y2pz2m19 = 9.0 * (x2 + 3.0 * y2 + z2) - 19.0;
		auto _9tx2py2p3z2m19 = 9.0 * (x2 + y2 + 3.0 * z2) - 19.0;
		auto _18x = 18.0 * x;
		auto _18y = 18.0 * y;
		auto _18z = 18.0 * z;

		auto _3m9x2 = 3.0 - 9.0 * x2;
		auto _3m9y2 = 3.0 - 9.0 * y2;
		auto _3m9z2 = 3.0 - 9.0 * z2;

		auto _2x = 2.0 * x;
		auto _2y = 2.0 * y;
		auto _2z = 2.0 * z;

		auto _18xm9t3x2py2pz2m19 = _18x - _9t3x2py2pz2m19;
		auto _18xp9t3x2py2pz2m19 = _18x + _9t3x2py2pz2m19;
		auto _18ym9tx2p3y2pz2m19 = _18y - _9tx2p3y2pz2m19;
		auto _18yp9tx2p3y2pz2m19 = _18y + _9tx2p3y2pz2m19;
		auto _18zm9tx2py2p3z2m19 = _18z - _9tx2py2p3z2m19;
		auto _18zp9tx2py2p3z2m19 = _18z + _9tx2py2p3z2m19;

		dN(0, 0) = _18xm9t3x2py2pz2m19 * _1myt1mz;
		dN(0, 1) = _1mxt1mz * _18ym9tx2p3y2pz2m19;
		dN(0, 2) = _1mxt1my * _18zm9tx2py2p3z2m19;
		dN(1, 0) = _18xp9t3x2py2pz2m19 * _1myt1mz;
		dN(1, 1) = _1pxt1mz * _18ym9tx2p3y2pz2m19;
		dN(1, 2) = _1pxt1my * _18zm9tx2py2p3z2m19;
		dN(2, 0) = _18xm9t3x2py2pz2m19 * _1pyt1mz;
		dN(2, 1) = _1mxt1mz * _18yp9tx2p3y2pz2m19;
		dN(2, 2) = _1mxt1py * _18zm9tx2py2p3z2m19;
		dN(3, 0) = _18xp9t3x2py2pz2m19 * _1pyt1mz;
		dN(3, 1) = _1pxt1mz * _18yp9tx2p3y2pz2m19;
		dN(3, 2) = _1pxt1py * _18zm9tx2py2p3z2m19;
		dN(4, 0) = _18xm9t3x2py2pz2m19 * _1myt1pz;
		dN(4, 1) = _1mxt1pz * _18ym9tx2p3y2pz2m19;
		dN(4, 2) = _1mxt1my * _18zp9tx2py2p3z2m19;
		dN(5, 0) = _18xp9t3x2py2pz2m19 * _1myt1pz;
		dN(5, 1) = _1pxt1pz * _18ym9tx2p3y2pz2m19;
		dN(5, 2) = _1pxt1my * _18zp9tx2py2p3z2m19;
		dN(6, 0) = _18xm9t3x2py2pz2m19 * _1pyt1pz;
		dN(6, 1) = _1mxt1pz * _18yp9tx2p3y2pz2m19;
		dN(6, 2) = _1mxt1py * _18zp9tx2py2p3z2m19;
		dN(7, 0) = _18xp9t3x2py2pz2m19 * _1pyt1pz;
		dN(7, 1) = _1pxt1pz * _18yp9tx2p3y2pz2m19;
		dN(7, 2) = _1pxt1py * _18zp9tx2py2p3z2m19;

		dN.topRows(8) /= 64.0;

		auto _m3m9x2m2x = -_3m9x2 - _2x;
		auto _p3m9x2m2x = _3m9x2 - _2x;
		auto _1mx2t1m3x = _1mx2 * _1m3x;
		auto _1mx2t1p3x = _1mx2 * _1p3x;
		dN(8, 0) = _m3m9x2m2x * _1myt1mz,
			  dN(8, 1) = -_1mx2t1m3x * _1mz,
			  dN(8, 2) = -_1mx2t1m3x * _1my;
		dN(9, 0) = _p3m9x2m2x * _1myt1mz,
			  dN(9, 1) = -_1mx2t1p3x * _1mz,
			  dN(9, 2) = -_1mx2t1p3x * _1my;
		dN(10, 0) = _m3m9x2m2x * _1myt1pz,
			   dN(10, 1) = -_1mx2t1m3x * _1pz,
			   dN(10, 2) = _1mx2t1m3x * _1my;
		dN(11, 0) = _p3m9x2m2x * _1myt1pz,
			   dN(11, 1) = -_1mx2t1p3x * _1pz,
			   dN(11, 2) = _1mx2t1p3x * _1my;
		dN(12, 0) = _m3m9x2m2x * _1pyt1mz,
			   dN(12, 1) = _1mx2t1m3x * _1mz,
			   dN(12, 2) = -_1mx2t1m3x * _1py;
		dN(13, 0) = _p3m9x2m2x * _1pyt1mz,
			   dN(13, 1) = _1mx2t1p3x * _1mz,
			   dN(13, 2) = -_1mx2t1p3x * _1py;
		dN(14, 0) = _m3m9x2m2x * _1pyt1pz,
			   dN(14, 1) = _1mx2t1m3x * _1pz,
			   dN(14, 2) = _1mx2t1m3x * _1py;
		dN(15, 0) = _p3m9x2m2x * _1pyt1pz,
			   dN(15, 1) = _1mx2t1p3x * _1pz,
			   dN(15, 2) = _1mx2t1p3x * _1py;

		auto _m3m9y2m2y = -_3m9y2 - _2y;
		auto _p3m9y2m2y = _3m9y2 - _2y;
		auto _1my2t1m3y = _1my2 * _1m3y;
		auto _1my2t1p3y = _1my2 * _1p3y;
		dN(16, 0) = -_1my2t1m3y * _1mz,
			   dN(16, 1) = _m3m9y2m2y * _1mxt1mz,
			   dN(16, 2) = -_1my2t1m3y * _1mx;
		dN(17, 0) = -_1my2t1p3y * _1mz,
			   dN(17, 1) = _p3m9y2m2y * _1mxt1mz,
			   dN(17, 2) = -_1my2t1p3y * _1mx;
		dN(18, 0) = _1my2t1m3y * _1mz,
			   dN(18, 1) = _m3m9y2m2y * _1pxt1mz,
			   dN(18, 2) = -_1my2t1m3y * _1px;
		dN(19, 0) = _1my2t1p3y * _1mz,
			   dN(19, 1) = _p3m9y2m2y * _1pxt1mz,
			   dN(19, 2) = -_1my2t1p3y * _1px;
		dN(20, 0) = -_1my2t1m3y * _1pz,
			   dN(20, 1) = _m3m9y2m2y * _1mxt1pz,
			   dN(20, 2) = _1my2t1m3y * _1mx;
		dN(21, 0) = -_1my2t1p3y * _1pz,
			   dN(21, 1) = _p3m9y2m2y * _1mxt1pz,
			   dN(21, 2) = _1my2t1p3y * _1mx;
		dN(22, 0) = _1my2t1m3y * _1pz,
			   dN(22, 1) = _m3m9y2m2y * _1pxt1pz,
			   dN(22, 2) = _1my2t1m3y * _1px;
		dN(23, 0) = _1my2t1p3y * _1pz,
			   dN(23, 1) = _p3m9y2m2y * _1pxt1pz,
			   dN(23, 2) = _1my2t1p3y * _1px;

		auto _m3m9z2m2z = -_3m9z2 - _2z;
		auto _p3m9z2m2z = _3m9z2 - _2z;
		auto _1mz2t1m3z = _1mz2 * _1m3z;
		auto _1mz2t1p3z = _1mz2 * _1p3z;
		dN(24, 0) = -_1mz2t1m3z * _1my,
			   dN(24, 1) = -_1mz2t1m3z * _1mx,
			   dN(24, 2) = _m3m9z2m2z * _1mxt1my;
		dN(25, 0) = -_1mz2t1p3z * _1my,
			   dN(25, 1) = -_1mz2t1p3z * _1mx,
			   dN(25, 2) = _p3m9z2m2z * _1mxt1my;
		dN(26, 0) = -_1mz2t1m3z * _1py,
			   dN(26, 1) = _1mz2t1m3z * _1mx,
			   dN(26, 2) = _m3m9z2m2z * _1mxt1py;
		dN(27, 0) = -_1mz2t1p3z * _1py,
			   dN(27, 1) = _1mz2t1p3z * _1mx,
			   dN(27, 2) = _p3m9z2m2z * _1mxt1py;
		dN(28, 0) = _1mz2t1m3z * _1my,
			   dN(28, 1) = -_1mz2t1m3z * _1px,
			   dN(28, 2) = _m3m9z2m2z * _1pxt1my;
		dN(29, 0) = _1mz2t1p3z * _1my,
			   dN(29, 1) = -_1mz2t1p3z * _1px,
			   dN(29, 2) = _p3m9z2m2z * _1pxt1my;
		dN(30, 0) = _1mz2t1m3z * _1py,
			   dN(30, 1) = _1mz2t1m3z * _1px,
			   dN(30, 2) = _m3m9z2m2z * _1pxt1py;
		dN(31, 0) = _1mz2t1p3z * _1py,
			   dN(31, 1) = _1mz2t1p3z * _1px,
			   dN(31, 2) = _p3m9z2m2z * _1pxt1py;

		dN.bottomRows(32u - 8u) *= 9.0 / 64.0;
	}

	return res;
}

// Determines Morten value according to z-curve.
inline uint64_t
zValue(Vector3d const &x, double invCellSize)
{
	std::array<int, 3> key;
	for (unsigned int i(0); i < 3; ++i)
	{
		if (x[i] >= 0.0)
			key[i] = static_cast<int>(invCellSize * x[i]);
		else
			key[i] = static_cast<int>(invCellSize * x[i]) - 1;
	}

	std::array<unsigned int, 3> p = {
		static_cast<unsigned int>(static_cast<int64_t>(key[0]) - (std::numeric_limits<int>::lowest() + 1)),
		static_cast<unsigned int>(static_cast<int64_t>(key[1]) - (std::numeric_limits<int>::lowest() + 1)),
		static_cast<unsigned int>(static_cast<int64_t>(key[2]) - (std::numeric_limits<int>::lowest() + 1))};

	return morton_lut(p);
}
} // namespace

Vector3d
CubicLagrangeDiscreteGrid::indexToNodePosition(unsigned int l) const
{
	auto x = Vector3d{};

	auto n = Matrix<unsigned int, 3, 1>::Map(m_resolution.data());

	auto nv = (n[0] + 1) * (n[1] + 1) * (n[2] + 1);
	auto ne_x = (n[0] + 0) * (n[1] + 1) * (n[2] + 1);
	auto ne_y = (n[0] + 1) * (n[1] + 0) * (n[2] + 1);
	auto ne_z = (n[0] + 1) * (n[1] + 1) * (n[2] + 0);
	auto ne = ne_x + ne_y + ne_z;

	auto ijk = Matrix<unsigned int, 3, 1>{};
	if (l < nv)
	{
		ijk(2) = l / ((n[1] + 1) * (n[0] + 1));
		auto temp = l % ((n[1] + 1) * (n[0] + 1));
		ijk(1) = temp / (n[0] + 1);
		ijk(0) = temp % (n[0] + 1);

		x = m_domain.min() + m_cell_size.cwiseProduct(ijk.cast<double>());
	}
	else if (l < nv + 2 * ne_x)
	{
		l -= nv;
		auto e_ind = l / 2;
		ijk(2) = e_ind / ((n[1] + 1) * n[0]);
		auto temp = e_ind % ((n[1] + 1) * n[0]);
		ijk(1) = temp / n[0];
		ijk(0) = temp % n[0];

		x = m_domain.min() + m_cell_size.cwiseProduct(ijk.cast<double>());
		x(0) += (1.0 + static_cast<double>(l % 2)) / 3.0 * m_cell_size[0];
	}
	else if (l < nv + 2 * (ne_x + ne_y))
	{
		l -= (nv + 2 * ne_x);
		auto e_ind = l / 2;
		ijk(0) = e_ind / ((n[2] + 1) * n[1]);
		auto temp = e_ind % ((n[2] + 1) * n[1]);
		ijk(2) = temp / n[1];
		ijk(1) = temp % n[1];

		x = m_domain.min() + m_cell_size.cwiseProduct(ijk.cast<double>());
		x(1) += (1.0 + static_cast<double>(l % 2)) / 3.0 * m_cell_size[1];
	}
	else
	{
		l -= (nv + 2 * (ne_x + ne_y));
		auto e_ind = l / 2;
		ijk(1) = e_ind / ((n[0] + 1) * n[2]);
		auto temp = e_ind % ((n[0] + 1) * n[2]);
		ijk(0) = temp / n[2];
		ijk(2) = temp % n[2];

		x = m_domain.min() + m_cell_size.cwiseProduct(ijk.cast<double>());
		x(2) += (1.0 + static_cast<double>(l % 2)) / 3.0 * m_cell_size[2];
	}

	return x;
}

CubicLagrangeDiscreteGrid::CubicLagrangeDiscreteGrid(std::string const &filename)
{
	load(filename);
}

CubicLagrangeDiscreteGrid::CubicLagrangeDiscreteGrid(AlignedBox3d const &domain,
													 std::array<unsigned int, 3> const &resolution)
	: DiscreteGrid(domain, resolution)
{
}

void CubicLagrangeDiscreteGrid::save(std::string const &filename) const
{
	auto out = std::ofstream(filename, std::ios::binary);
	serialize::write(*out.rdbuf(), m_domain);
	serialize::write(*out.rdbuf(), m_resolution);
	serialize::write(*out.rdbuf(), m_cell_size);
	serialize::write(*out.rdbuf(), m_inv_cell_size);
	serialize::write(*out.rdbuf(), m_n_cells);
	serialize::write(*out.rdbuf(), m_n_fields);

	serialize::write(*out.rdbuf(), m_nodes.size());
	for (auto const &nodes : m_nodes)
	{
		serialize::write(*out.rdbuf(), nodes.size());
		for (auto const &node : nodes)
		{
			serialize::write(*out.rdbuf(), node);
		}
	}

	serialize::write(*out.rdbuf(), m_cells.size());
	for (auto const &cells : m_cells)
	{
		serialize::write(*out.rdbuf(), cells.size());
		for (auto const &cell : cells)
		{
			serialize::write(*out.rdbuf(), cell);
		}
	}

	serialize::write(*out.rdbuf(), m_cell_map.size());
	for (auto const &maps : m_cell_map)
	{
		serialize::write(*out.rdbuf(), maps.size());
		for (auto const &map : maps)
		{
			serialize::write(*out.rdbuf(), map);
		}
	}

	out.close();
}

void CubicLagrangeDiscreteGrid::load(std::string const &filename)
{
	auto in = std::ifstream(filename, std::ios::binary);

	if (!in.good())
	{
		std::cerr << "ERROR: Discrete grid can not be loaded. Input file does not exist!" << std::endl;
		return;
	}

	serialize::read(*in.rdbuf(), m_domain);
	serialize::read(*in.rdbuf(), m_resolution);
	serialize::read(*in.rdbuf(), m_cell_size);
	serialize::read(*in.rdbuf(), m_inv_cell_size);
	serialize::read(*in.rdbuf(), m_n_cells);
	serialize::read(*in.rdbuf(), m_n_fields);

	auto n_nodes = std::size_t{};
	serialize::read(*in.rdbuf(), n_nodes);
	m_nodes.resize(n_nodes);
	for (auto &nodes : m_nodes)
	{
		serialize::read(*in.rdbuf(), n_nodes);
		nodes.resize(n_nodes);
		for (auto &node : nodes)
		{
			serialize::read(*in.rdbuf(), node);
		}
	}

	auto n_cells = std::size_t{};
	serialize::read(*in.rdbuf(), n_cells);
	m_cells.resize(n_cells);
	for (auto &cells : m_cells)
	{
		serialize::read(*in.rdbuf(), n_cells);
		cells.resize(n_cells);
		for (auto &cell : cells)
		{
			serialize::read(*in.rdbuf(), cell);
		}
	}

	auto n_cell_maps = std::size_t{};
	serialize::read(*in.rdbuf(), n_cell_maps);
	m_cell_map.resize(n_cell_maps);
	for (auto &cell_maps : m_cell_map)
	{
		serialize::read(*in.rdbuf(), n_cell_maps);
		cell_maps.resize(n_cell_maps);
		for (auto &cell_map : cell_maps)
		{
			serialize::read(*in.rdbuf(), cell_map);
		}
	}

	in.close();
}

unsigned int
CubicLagrangeDiscreteGrid::addFunction(ContinuousFunction const &func, bool verbose,
									   SamplePredicate const &pred)
{
	using namespace std::chrono;

	auto t0_construction = high_resolution_clock::now();

	auto n = Matrix<unsigned int, 3, 1>::Map(m_resolution.data());

	auto nv = (n[0] + 1) * (n[1] + 1) * (n[2] + 1);
	auto ne_x = (n[0] + 0) * (n[1] + 1) * (n[2] + 1);
	auto ne_y = (n[0] + 1) * (n[1] + 0) * (n[2] + 1);
	auto ne_z = (n[0] + 1) * (n[1] + 1) * (n[2] + 0);
	auto ne = ne_x + ne_y + ne_z;

	auto n_nodes = nv + 2 * ne;

	m_nodes.push_back({});
	auto &coeffs = m_nodes.back();
	coeffs.resize(n_nodes);

	std::atomic_uint counter(0u);
	SpinLock mutex;
	auto t0 = high_resolution_clock::now();

#pragma omp parallel default(shared)
	{
#pragma omp for schedule(static) nowait
		for (int l = 0; l < static_cast<int>(n_nodes); ++l)
		{
			auto x = indexToNodePosition(l);
			auto &c = coeffs[l];

			if (!pred || pred(x))
				c = func(x);
			else
				c = std::numeric_limits<double>::max();

			if (verbose && (++counter == n_nodes || duration_cast<milliseconds>(high_resolution_clock::now() - t0).count() > 1000u))
			{
				std::async(std::launch::async, [&]() {
					mutex.lock();
					t0 = high_resolution_clock::now();
					std::cout << "\r"
							  << "Construction " << std::setw(20)
							  << 100.0 * static_cast<double>(counter) / static_cast<double>(n_nodes) << "%";
					mutex.unlock();
				});
			}
		}
	}

	m_cells.push_back({});
	auto &cells = m_cells.back();
	cells.resize(m_n_cells);
	for (auto l = 0u; l < m_n_cells; ++l)
	{
		auto k = l / (n[1] * n[0]);
		auto temp = l % (n[1] * n[0]);
		auto j = temp / n[0];
		auto i = temp % n[0];

		auto nx = n[0];
		auto ny = n[1];
		auto nz = n[2];

		auto &cell = cells[l];
		cell[0] = (nx + 1) * (ny + 1) * k + (nx + 1) * j + i;
		cell[1] = (nx + 1) * (ny + 1) * k + (nx + 1) * j + i + 1;
		cell[2] = (nx + 1) * (ny + 1) * k + (nx + 1) * (j + 1) + i;
		cell[3] = (nx + 1) * (ny + 1) * k + (nx + 1) * (j + 1) + i + 1;
		cell[4] = (nx + 1) * (ny + 1) * (k + 1) + (nx + 1) * j + i;
		cell[5] = (nx + 1) * (ny + 1) * (k + 1) + (nx + 1) * j + i + 1;
		cell[6] = (nx + 1) * (ny + 1) * (k + 1) + (nx + 1) * (j + 1) + i;
		cell[7] = (nx + 1) * (ny + 1) * (k + 1) + (nx + 1) * (j + 1) + i + 1;

		auto offset = nv;
		cell[8] = offset + 2 * (nx * (ny + 1) * k + nx * j + i);
		cell[9] = cell[8] + 1;
		cell[10] = offset + 2 * (nx * (ny + 1) * (k + 1) + nx * j + i);
		cell[11] = cell[10] + 1;
		cell[12] = offset + 2 * (nx * (ny + 1) * k + nx * (j + 1) + i);
		cell[13] = cell[12] + 1;
		cell[14] = offset + 2 * (nx * (ny + 1) * (k + 1) + nx * (j + 1) + i);
		cell[15] = cell[14] + 1;

		offset += 2 * ne_x;
		cell[16] = offset + 2 * (ny * (nz + 1) * i + ny * k + j);
		cell[17] = cell[16] + 1;
		cell[18] = offset + 2 * (ny * (nz + 1) * (i + 1) + ny * k + j);
		cell[19] = cell[18] + 1;
		cell[20] = offset + 2 * (ny * (nz + 1) * i + ny * (k + 1) + j);
		cell[21] = cell[20] + 1;
		cell[22] = offset + 2 * (ny * (nz + 1) * (i + 1) + ny * (k + 1) + j);
		cell[23] = cell[22] + 1;

		offset += 2 * ne_y;
		cell[24] = offset + 2 * (nz * (nx + 1) * j + nz * i + k);
		cell[25] = cell[24] + 1;
		cell[26] = offset + 2 * (nz * (nx + 1) * (j + 1) + nz * i + k);
		cell[27] = cell[26] + 1;
		cell[28] = offset + 2 * (nz * (nx + 1) * j + nz * (i + 1) + k);
		cell[29] = cell[28] + 1;
		cell[30] = offset + 2 * (nz * (nx + 1) * (j + 1) + nz * (i + 1) + k);
		cell[31] = cell[30] + 1;
	}

	m_cell_map.push_back({});
	auto &cell_map = m_cell_map.back();
	cell_map.resize(m_n_cells);
	std::iota(cell_map.begin(), cell_map.end(), 0u);

	if (verbose)
	{
		std::cout << "\rConstruction took " << std::setw(15) << static_cast<double>(duration_cast<milliseconds>(high_resolution_clock::now() - t0_construction).count()) / 1000.0 << "s" << std::endl;
	}

	return static_cast<unsigned int>(m_n_fields++);
}

bool
CubicLagrangeDiscreteGrid::determineShapeFunctions(unsigned int field_id, Eigen::Vector3d const &x,
	std::array<unsigned int, 32> &cell, Eigen::Vector3d &c0, Eigen::Matrix<double, 32, 1> &N,
	Eigen::Matrix<double, 32, 3> *dN) const
{
	if (!m_domain.contains(x))
		return false;

	auto mi = (x - m_domain.min()).cwiseProduct(m_inv_cell_size).cast<unsigned int>().eval();
	if (mi[0] >= m_resolution[0])
		mi[0] = m_resolution[0] - 1;
	if (mi[1] >= m_resolution[1])
		mi[1] = m_resolution[1] - 1;
	if (mi[2] >= m_resolution[2])
		mi[2] = m_resolution[2] - 1;
	auto i = multiToSingleIndex({ { mi(0), mi(1), mi(2) } });
	auto i_ = m_cell_map[field_id][i];
	if (i_ == std::numeric_limits<unsigned int>::max())
		return false;

	auto sd = subdomain(i);
	i = i_;
	auto d = sd.diagonal().eval();

	auto denom = (sd.max() - sd.min()).eval();
	c0 = Vector3d::Constant(2.0).cwiseQuotient(denom).eval();
	auto c1 = (sd.max() + sd.min()).cwiseQuotient(denom).eval();
	auto xi = (c0.cwiseProduct(x) - c1).eval();

	cell = m_cells[field_id][i];
	N = shape_function_(xi, dN);
	return true;
}

double 
CubicLagrangeDiscreteGrid::interpolate(unsigned int field_id, Eigen::Vector3d const& xi, const std::array<unsigned int, 32> &cell, const Eigen::Vector3d &c0, const Eigen::Matrix<double, 32, 1> &N,
	Eigen::Vector3d* gradient, Eigen::Matrix<double, 32, 3> *dN) const
{
	if (!gradient)
	{
		auto phi = 0.0;
		for (auto j = 0u; j < 32u; ++j)
		{
			auto v = cell[j];
			auto c = m_nodes[field_id][v];
			if (c == std::numeric_limits<double>::max())
			{
				return std::numeric_limits<double>::max();
			}
			phi += c * N[j];
		}

		return phi;
	}

	auto phi = 0.0;
	gradient->setZero();
	for (auto j = 0u; j < 32u; ++j)
	{
		auto v = cell[j];
		auto c = m_nodes[field_id][v];
		if (c == std::numeric_limits<double>::max())
		{
			gradient->setZero();
			return std::numeric_limits<double>::max();
		}
		phi += c * N[j];
		(*gradient)(0) += c * (*dN)(j, 0);
		(*gradient)(1) += c * (*dN)(j, 1);
		(*gradient)(2) += c * (*dN)(j, 2);
	}
	gradient->array() *= c0.array();

	return phi;
}

double
CubicLagrangeDiscreteGrid::interpolate(unsigned int field_id, Vector3d const &x,
									   Vector3d *gradient) const
{
	if (!m_domain.contains(x))
		return std::numeric_limits<double>::max();

	auto mi = (x - m_domain.min()).cwiseProduct(m_inv_cell_size).cast<unsigned int>().eval();
	if (mi[0] >= m_resolution[0])
		mi[0] = m_resolution[0] - 1;
	if (mi[1] >= m_resolution[1])
		mi[1] = m_resolution[1] - 1;
	if (mi[2] >= m_resolution[2])
		mi[2] = m_resolution[2] - 1;
	auto i = multiToSingleIndex({{mi(0), mi(1), mi(2)}});
	auto i_ = m_cell_map[field_id][i];
	if (i_ == std::numeric_limits<unsigned int>::max())
		return std::numeric_limits<double>::max();

	auto sd = subdomain(i);
	i = i_;
	auto d = sd.diagonal().eval();

	auto denom = (sd.max() - sd.min()).eval();
	auto c0 = Vector3d::Constant(2.0).cwiseQuotient(denom).eval();
	auto c1 = (sd.max() + sd.min()).cwiseQuotient(denom).eval();
	auto xi = (c0.cwiseProduct(x) - c1).eval();

	auto const &cell = m_cells[field_id][i];
	if (!gradient)
	{
		//auto phi = m_coefficients[field_id][i].dot(shape_function_(xi, nullptr));
		auto phi = 0.0;
		auto N = shape_function_(xi, nullptr);
		for (auto j = 0u; j < 32u; ++j)
		{
			auto v = cell[j];
			auto c = m_nodes[field_id][v];
			if (c == std::numeric_limits<double>::max())
			{
				return std::numeric_limits<double>::max();
			}
			phi += c * N[j];
		}

		return phi;
	}

	auto dN = Matrix<double, 32, 3>{};
	auto N = shape_function_(xi, &dN);

	// TEST
	//auto eps = 1.0e-6;
	//auto ndN = Matrix<double, 32, 3>{};
	//for (auto j = 0u; j < 3u; ++j)
	//{
	//    auto xip = xi;
	//    xip(j) += eps;
	//    auto xim = xi;
	//    xim(j) -= eps;
	//    auto Np = shape_function_(xip, nullptr);
	//    auto Nm = shape_function_(xim, nullptr);
	//    ndN.col(j) = (Np - Nm) / (2.0 * eps);
	//}
	//std::cout << (dN - ndN).cwiseAbs().maxCoeff() /*/ (dN.maxCoeff())*/ << std::endl;
	///

	auto phi = 0.0;
	gradient->setZero();
	for (auto j = 0u; j < 32u; ++j)
	{
		auto v = cell[j];
		auto c = m_nodes[field_id][v];
		if (c == std::numeric_limits<double>::max())
		{
			gradient->setZero();
			return std::numeric_limits<double>::max();
		}
		phi += c * N[j];
		(*gradient)(0) += c * dN(j, 0);
		(*gradient)(1) += c * dN(j, 1);
		(*gradient)(2) += c * dN(j, 2);
	}
	gradient->array() *= c0.array();

	return phi;
}

void CubicLagrangeDiscreteGrid::reduceField(unsigned int field_id, Predicate pred)
{
	auto &coeffs = m_nodes[field_id];
	auto &cells = m_cells[field_id];
	auto keep = std::vector<bool>(coeffs.size());
	for (auto l = 0u; l < coeffs.size(); ++l)
	{
		auto xi = indexToNodePosition(l);
		keep[l] = pred(xi, coeffs[l]) && coeffs[l] != std::numeric_limits<double>::max();
	}

	auto &cell_map = m_cell_map[field_id];
	cell_map.resize(m_n_cells);
	std::iota(cell_map.begin(), cell_map.end(), 0u);

	auto cells_ = cells;
	cells.clear();
	for (auto i = 0u; i < cells_.size(); ++i)
	{
		auto keep_cell = false;
		auto vals = std::vector<double>{};
		for (auto v : cells_[i])
		{
			keep_cell |= keep[v];
			vals.push_back(coeffs[v]);
		}
		if (keep_cell)
		{
			cells.push_back(cells_[i]);
			cell_map[i] = static_cast<unsigned int>(cells.size() - 1);
		}
		else
			cell_map[i] = std::numeric_limits<unsigned int>::max();
	}

	auto n = Matrix<unsigned int, 3, 1>::Map(m_resolution.data());

	auto nv = (n[0] + 1) * (n[1] + 1) * (n[2] + 1);
	auto ne_x = (n[0] + 0) * (n[1] + 1) * (n[2] + 1);
	auto ne_y = (n[0] + 1) * (n[1] + 0) * (n[2] + 1);
	auto ne_z = (n[0] + 1) * (n[1] + 1) * (n[2] + 0);
	auto ne = ne_x + ne_y + ne_z;

	// Reduce vertices.
	auto xi = Vector3d{};
	auto z_values = std::vector<uint64_t>(coeffs.size());
	for (auto l = 0u; l < coeffs.size(); ++l)
	{
		auto xi = indexToNodePosition(l);
		z_values[l] = zValue(xi, 4.0 * m_inv_cell_size.minCoeff());
	}

	std::fill(keep.begin(), keep.end(), false);

	auto vertex_to_cell = std::vector<std::set<std::pair<unsigned int, unsigned int>>>(coeffs.size());
	for (auto c = 0u; c < cells.size(); ++c)
	{
		auto const &cell = cells[c];

		for (auto j = 0u; j < cell.size(); ++j)
		{
			auto v = cell[j];
			keep[v] = true;
			vertex_to_cell[v].insert({c, j});
		}
	}
	auto last_vertex = static_cast<unsigned int>(coeffs.size() - 1);
	for (auto i = static_cast<int>(coeffs.size() - 1); i >= 0; --i)
	{
		if (!keep[i])
		{
			std::swap(coeffs[i], coeffs[last_vertex]);
			std::swap(z_values[i], z_values[last_vertex]);
			std::swap(vertex_to_cell[i], vertex_to_cell[last_vertex]);
			for (auto const &kvp : vertex_to_cell[i])
			{
				cells[kvp.first][kvp.second] = i;
			}
			for (auto const &kvp : vertex_to_cell[last_vertex])
			{
				cells[kvp.first][kvp.second] = last_vertex;
			}

			last_vertex--;
		}
	}
	coeffs.resize(last_vertex + 1);
	z_values.resize(coeffs.size());

	auto sort_pattern = std::vector<unsigned int>(coeffs.size());
	std::iota(sort_pattern.begin(), sort_pattern.end(), 0u);
	std::sort(sort_pattern.begin(), sort_pattern.end(),
			  [&](unsigned int i, unsigned int j) {
				  return z_values[i] < z_values[j];
			  });

	for (auto i = 0u; i < sort_pattern.size(); ++i)
	{
		auto j = sort_pattern[i];
		for (auto const &kvp : vertex_to_cell[j])
		{
			assert(cells[kvp.first][kvp.second] == j);
			cells[kvp.first][kvp.second] = i;
		}
	}

	auto coeffs_ = coeffs;
	std::transform(sort_pattern.begin(), sort_pattern.end(), coeffs.begin(),
				   [&coeffs_](unsigned int i) { return coeffs_[i]; });
}

void CubicLagrangeDiscreteGrid::forEachCell(unsigned int field_id,
											std::function<void(unsigned int, AlignedBox3d const &, unsigned int)> const &cb) const
{
	auto n = m_resolution[0] * m_resolution[1] * m_resolution[2];
	for (auto i = 0u; i < n; ++i)
	{
		auto domain = AlignedBox3d{};
		auto mi = singleToMultiIndex(i);
		domain.min() = m_domain.min() + Matrix<unsigned int, 3, 1>::Map(mi.data()).cast<double>().cwiseProduct(m_cell_size);
		domain.max() = domain.min() + m_cell_size;

		cb(i, domain, 0);
	}
}

} // namespace Discregrid
