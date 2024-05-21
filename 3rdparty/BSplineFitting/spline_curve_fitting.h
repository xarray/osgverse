
#include "cubic_b_spline.h"
#include "open_cubic_b_spline.h"

/********************************************************************
	created:	 2009/07/23
	created:	 23:7:2009   19:13
	file base:	 spline_fitting.h
	author:		 Zheng Qian
    contact:     qian.zheng@siat.ac.cn
    affiliation: Shenzhen Institute of Advanced Technology
	
	purpose:	 cubic spline curve fitting  a set of 2d points
	             This is an implementation of this paper: 
				 http://www.geometrie.tuwien.ac.at/ig/sn/2006/wpl_curves_06/wpl_curves_06.html
*********************************************************************/


class  SplineCurveFitting
{
public:
	SplineCurveFitting(void){}
	~SplineCurveFitting(void){}

	//////////////////////////////////////////////////////////////////////////
	// Fitting B-Spline Curves to Point Clouds 
	// refer : Fitting B-Spline Curve to Point Clouds by Curvature-Based Squared Distance Minimization
	// controlNum: the number of control points
	// alpha:      the coefficient of curvature constraint
	// beta:       the coefficient of curve length constraint
	// maxIterNum: the maximum of iteration
	// eplison:    the threshold of ending iteration
	//////////////////////////////////////////////////////////////////////////
	double fitAClosedCurve( const vector<Vector2d>& points, 
		ClosedCubicBSplineCurve& curve,
		int controlNum  = 28,
		int maxIterNum = 30, 
		double alpha  = 0.002,
		double beta = 0.005,  // 0.005	     
		double eplison = 0.0001);


	double fitAOpenCurve( const vector<Vector2d>& points, 
		OpenCubicBSplineCurve& curve,
		int controlNum  = 28,
		int maxIterNum = 30, 
		double alpha  = 0.002,
		double beta = 0.005,  // 0.005	     
		double eplison = 0.0001);

private:
	// initial control points
	void initClosedControlPoints( const vector<Vector2d>& points, 
		vector<Vector2d>& controlPs,
		int controlNum);


	void initOpenControlPoints(const vector<Vector2d>& points, 
		vector<Vector2d>& controlPs,
		int controlNum);
};