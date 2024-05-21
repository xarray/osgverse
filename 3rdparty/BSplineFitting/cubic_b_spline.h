#pragma  once


/********************************************************************
created:	 2009/07/23
created:	 23:7:2009   19:17
file base:	 cubic_b_spline.h
author:		 Zheng Qian
contact:     qian.zheng@siat.ac.cn
affiliation: Shenzhen Institute of Advanced Technology
purpose:	 cubic B spline  
reference:   http://en.wikipedia.org/wiki/B-spline
refined:     2014/08/05

*********************************************************************/

#include <Eigen/Core>
#include <vector>
#include <map>

using namespace std;
using namespace Eigen;



class ClosedCubicBSplineCurve 
{

public:
	typedef std::pair<int, double> Parameter;

	ClosedCubicBSplineCurve( double interal = 0.001 )
		: interal_(interal)
	{

	}

	~ClosedCubicBSplineCurve() { 
		clear(); 
	}

	unsigned int nb_control()  const { return controls_.size(); }

	//////////////////////////////////////////////////////////////////////////
	// compute the x ,y position of current parameter
	//////////////////////////////////////////////////////////////////////////
	Vector2d getPos(const Parameter& para) const;

	//////////////////////////////////////////////////////////////////////////
	// compute the first differential
	//////////////////////////////////////////////////////////////////////////
	Vector2d getFirstDiff( const Parameter& para) const ;

	//////////////////////////////////////////////////////////////////////////
	// compute the second differential
	//////////////////////////////////////////////////////////////////////////
	Vector2d getSecondDiff( const Parameter& para) const ;

	//////////////////////////////////////////////////////////////////////////
	// compute the curvature
	//////////////////////////////////////////////////////////////////////////
	double getCurvature( const Parameter& para) const ;


	//////////////////////////////////////////////////////////////////////////
	// compute the unit tangent vector
	//////////////////////////////////////////////////////////////////////////
	Vector2d getTangent( const Parameter &para ) const ;


	//////////////////////////////////////////////////////////////////////////
	// compute the unit Normal vector
	//////////////////////////////////////////////////////////////////////////
	Vector2d getNormal( const Parameter &para) const;

	//////////////////////////////////////////////////////////////////////////
	// compute the Curvature center ( rho = k)
	//////////////////////////////////////////////////////////////////////////
	Vector2d getCurvCenter( const Parameter &para) const;

	///////////////////////////////////////////////////////////////////////////
	// compute the foot print
	//////////////////////////////////////////////////////////////////////////
	double findFootPrint( const vector<Vector2d>& givepoints, 
		vector<Parameter>& footPrints) const ;

	//////////////////////////////////////////////////////////////////////////
	// find the coff vector
	//////////////////////////////////////////////////////////////////////////
	VectorXd getCoffe( const Parameter& para) const ;

	//////////////////////////////////////////////////////////////////////////
	// set the control points and compute a uniform spatial partition of the data points
	//////////////////////////////////////////////////////////////////////////
	void setNewControl( const vector<Vector2d>& controlPs);

	//////////////////////////////////////////////////////////////////////////
	// check if two point is on same side. para is foot print of p1
	//////////////////////////////////////////////////////////////////////////
	bool checkSameSide( Vector2d p1,  Vector2d p2, Vector2d neip);



	const vector<Vector2d>& getControls() const {return controls_;}
	const vector<Vector2d>& getSamples() const { return positions_; }



	MatrixXd getSIntegralSq( );

	MatrixXd getFIntegralSq( );

	void getDistance_sd( const Vector2d& point, const Parameter& para, MatrixXd& ehm, VectorXd& ehv );


	//************************************
	// Method:    local2GlobalIdx
	// Returns:   int
	// Function:  由localIdx变换globalIdx
	// Time:      2015/07/10
	// Author:    Qian
	//************************************
	int local2GlobalIdx( int segId, int localIdx);






private:
	void clear()
	{
		controls_.clear();
		positions_.clear();
	}

	Parameter  getPara( int index ) const ;

	//////////////////////////////////////////////////////////////////////////
	//winding number test for a point in a polygon
	// softSurfer (www.softsurfer.com)
	//////////////////////////////////////////////////////////////////////////
	bool checkInside( Vector2d p);

	//////////////////////////////////////////////////////////////////////////
	// tests if a point is Left|On|Right of an infinite line.
	//////////////////////////////////////////////////////////////////////////
	int isLeft( Vector2d p0, Vector2d p1, Vector2d p2);


private:
	double interal_;                //采样点的间隔
	std::vector<Vector2d> controls_;    //曲线的控制点
	std::vector<Vector2d> positions_;   //曲线上的采样点

};

