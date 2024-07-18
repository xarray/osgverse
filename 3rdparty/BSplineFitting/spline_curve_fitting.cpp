#include "spline_curve_fitting.h"
#include <Eigen/SVD>


#include <iostream>


void SplineCurveFitting::initClosedControlPoints(const vector<Vector2d>& points, 
									  vector<Vector2d>& controlPs,
									  int controlNum)
{
	const double C_M_PI =3.14159265358979323846;
	// compute the initial 12 control points
	controlPs.clear();

	Vector2d min_point = points[0];
	Vector2d max_point = points[0];

	for( unsigned int i = 0 ;i!= points.size(); ++i) {
		Vector2d v = points[i];
		if( min_point.x() > v.x() )  min_point.x() = v.x();
		if( min_point.y() > v.y() )  min_point.y() = v.y();
		if( max_point.x() < v.x() )  max_point.x() = v.x();
		if( max_point.y() < v.y() )  max_point.y() = v.y();
	}

	Vector2d dir = ( max_point-min_point )*0.5;
	Vector2d cent = min_point + dir;


	Vector2d center = min_point + (max_point - min_point)*0.5;
	double radius = (max_point - min_point).norm()/2.0;

	double delta = 2*C_M_PI/controlNum;

	for(size_t i = 0; i < controlNum; i++)
	{
		Vector2d point = center+Vector2d(radius*std::cos(i*delta),radius*std::sin(i*delta));
		controlPs.push_back(point);
  }



// 	v1 = cent - 1.05*dir;
// 	v2 = cent + 1.05*dir;
// 
// 	vector<Vector2d> rets;
// 	rets.push_back( v1 );
// 	rets.push_back( Vector2d( v1.x(), v2.y() ) );
// 	rets.push_back( v2 );
// 	rets.push_back( Vector2d( v2.x(), v1.y() ) );
// 	rets.push_back( v1 );
// 
// 
// 	for( int i = 0; i < 4; i++ )
// 	{
// 		Vector2d p1 = rets[i];
// 		Vector2d p2 = rets[i+1];
// 		for(int j =0; j < perNum; j++) {
// 			controlPs.push_back(  p1 + (p2-p1) * j/(double)(perNum));
// 		}
// 	}

}

double SplineCurveFitting::fitAClosedCurve(
						   const vector<Vector2d> &points, 
						   ClosedCubicBSplineCurve &curve,
						   int controlNum /* = 28 */,
						   int maxIterNum  /*= 30*/,
						   double alpha /* = 0.002*/, 
						   double gama /* = 0.002 */,
						   double eplison /* = 0.0001*/)
{
	controlNum = controlNum/4*4;

	// initialize the cube B-spline
	ClosedCubicBSplineCurve* spline = &curve;
	vector<Vector2d> controlPs;
	initClosedControlPoints(points, controlPs, controlNum);
	spline->setNewControl( controlPs);

	// update the control point
	// compute P"(t)
	MatrixXd pm = spline->getSIntegralSq();
	MatrixXd sm = spline->getFIntegralSq();
	// end test
	
	// find the foot print, will result in error
	std::vector< std::pair<int,double> > parameters;
	double fsd = spline->findFootPrint( points, parameters);
	int iterNum = 0;
	while( fsd > eplison && iterNum < maxIterNum)
	{
		MatrixXd ehm(2*controlNum, 2*controlNum);
		VectorXd ehv( 2*controlNum);

		ehm.setZero();
		ehv.setZero();

		for( int i = 0; i!= (int)parameters.size(); ++i)
		{
			spline->getDistance_sd( points[i], parameters[i], ehm, ehv );
		}

// 		MatrixXd tmpm( 2*controlNum, 2*controlNum );
// 		VectorXd tmpv( 2*controlNum );
// 		tmpm.setZero();
// 		tmpv.setZero();

// 		// compute h(D)
// 		for( int i = 0; i< (int)parameters.size(); i++)
// 		{
// 
// 			// compute d, rho, Tkv, Nkv
// 			double kappa = spline->getCurvature( parameters[i] );
// 			double rho = 10e+6;
// 			Vector2d neip = spline->getPos( parameters[i]);
// 			Vector2d Tkv = spline->getTangent( parameters[i]);
// 			Vector2d Nkv = spline->getNormal( parameters[i]);
// 			double d =  ( points[i] - neip ).norm() ;
// 			Vector2d Kv(0.0,0.0);
// 			bool sign = true;
// 			if( kappa != 0.0f )
// 			{
// 				rho = 1/kappa;
// 				Kv = spline->getCurvCenter( parameters[i] );
// 				double ddd =  ( Kv - neip ).norm() ;
// 				sign = spline->checkSameSide( Kv, points[i], neip);
// 			}
// 	
//  			VectorXd coffv = spline->getCoffe( parameters[i]);
// 			MatrixXd tempcoffm1( controlNum,1);
// 			for( int ij = 0; ij < controlNum; ij++)
// 				tempcoffm1(ij,0) = coffv[ij];
// 			MatrixXd tempcoffm = tempcoffm1 * (tempcoffm1.transpose());
// 
// 			// update the matrix
// 			double fxx  = Tkv.x()*Tkv.x();
// 			double fyy = Tkv.y()*Tkv.y();
// 			double fxy = Tkv.x()*Tkv.y();
// 
// 			Vector2d oldp = neip - points[i];
// 
// 			if( !sign)
// 			{
// 				d = -d;
// 				VectorXd tempv1 = (coffv)* ( d/(d-rho) ) *( fxx*points[i].x() + fxy*points[i].y());
// 				VectorXd tempv2 = (coffv)* ( d/(d-rho) ) *( fyy*points[i].y() + fxy*points[i].x());
// 				for(int i2= 0; i2< controlNum; i2++)
// 				{
// 					for( int j = 0; j < controlNum; j++)
// 					{
// 						double fp = ( d/(d-rho) )*tempcoffm(i2,j);
// 						ehm(i2,j) += fxx* fp;
// 						ehm(i2,j+controlNum) += fxy * fp;
// 						ehm(i2+controlNum,j) += fxy * fp;
// 						ehm(i2+controlNum,j+controlNum) += fyy *fp;
// 					}
// 					ehv[i2] += tempv1[i2];
// 					ehv[i2+controlNum] += tempv2[i2];
// 				}
//  			}
// 			fxx  = Nkv.x()*Nkv.x();
// 			fyy = Nkv.y()*Nkv.y();
// 			fxy = Nkv.x()*Nkv.y();
// 			VectorXd tempv1 = (coffv)*( fxx*points[i].x() + fxy*points[i].y());
// 			VectorXd tempv2 = (coffv)*( fyy*points[i].y() + fxy*points[i].x());
// 			for( int i2= 0; i2< controlNum; i2++)
// 			{
// 				for( int j = 0; j < controlNum; j++)
// 				{
// 					double fp = tempcoffm(i2,j); 
// 					ehm(i2,j) += fxx* fp;
// 					ehm(i2,j+controlNum) += fxy * fp;
// 					ehm(i2+controlNum,j) += fxy * fp;
// 					ehm(i2+controlNum,j+controlNum) += fyy *fp;
// 				}
// 				ehv[i2] += tempv1[i2];
// 				ehv[i2+controlNum] += tempv2[i2];
//  			}
// 
// 			spline->getDistance_sd( points[i],parameters[i], tmpm, tmpv);
// 
// 			double errorm = (tmpm/2 - ehm).norm();
// 			std::cout << errorm << std::endl;
// 		}
 

		// check if ehm, ehv right
		//solve the function
		MatrixXd fm = ehm*0.5 + pm*alpha + sm*gama; 
		VectorXd ehv2 = ehv*0.5;
		JacobiSVD<MatrixXd> svd(fm, ComputeThinU | ComputeThinV);
		VectorXd resultxy = svd.solve(ehv2);

		// update the curve
		for( int i = 0; i<controlNum; i++)
			controlPs[i] = Vector2d( resultxy[i], resultxy[i+controlNum]);
		spline->setNewControl( controlPs );	
		++ iterNum;

		fsd = spline->findFootPrint( points, parameters );
	}

	return fsd;
}




//************************************
// Method:    initOpenControlPoints
// Returns:   void
// Function:  需要修改，如果初始控制点的位置偏差太大，会出现问题
// Time:      2015/07/13
// Author:    Qian
//************************************
void SplineCurveFitting::initOpenControlPoints( const vector<Vector2d>& points, vector<Vector2d>& controlPs, int controlNum )
{	
	// compute the initial control points
	controlPs.clear();

	double gap = double(points.size())/(controlNum-2);
	controlPs.push_back( points[0] );
	for( int i = 0 ;i!= (controlNum-2); ++i )
	{
		controlPs.push_back( points[std::floor(i*gap)] );
	}
	controlPs.push_back( *points.rbegin());

// 	Vector2d min_point = points[0];
// 	Vector2d max_point = points[0];
// 
// 	for( unsigned int i = 0 ;i!= points.size(); ++i) {
// 		Vector2d v = points[i];
// 		if( min_point.x() > v.x() )  min_point.x() = v.x();
// 		if( min_point.y() > v.y() )  min_point.y() = v.y();
// 		if( max_point.x() < v.x() )  max_point.x() = v.x();
// 		if( max_point.y() < v.y() )  max_point.y() = v.y();
// 	}

// 	//circle
// 	const double C_M_PI =3.14159265358979323846;
// 	Vector2d dir = ( max_point-min_point )*0.5;
// 	Vector2d cent = min_point + dir;
// 
// 	Vector2d center = min_point + (max_point - min_point)*0.5;
// 	double radius = (max_point - min_point).norm()/2.0;
// 	double delta = 2*C_M_PI/controlNum;
// 
// 	for(size_t i = 0; i < controlNum; i++)
// 	{
// 		Vector2d point = center+Vector2d(radius*std::cos(i*delta),radius*std::sin(i*delta));
// 		controlPs.push_back(point);
// 	}

// 	for(int j =0; j < controlNum; j++) {
// 		controlPs.push_back(  min_point + (max_point-min_point) * j/(double)(controlNum));
// 	}	

}

double SplineCurveFitting::fitAOpenCurve( const vector<Vector2d>& points, 
										 OpenCubicBSplineCurve& curve, 
										 int controlNum /*= 28*/, int maxIterNum /*= 30*/, double alpha /*= 0.002*/, double beta /*= 0.005*/, /* 0.005 */ double eplison /*= 0.0001*/ )
{
	// initialize the cube B-spline
	OpenCubicBSplineCurve* spline = &curve;
	vector<Vector2d> controlPs;
	initOpenControlPoints(points, controlPs, controlNum);
	spline->setNewControl( controlPs);

	// update the control point
	// compute P"(t)
	MatrixXd pm = spline->getSIntegralSq();
	MatrixXd sm = spline->getFIntegralSq();
	// end test

	// find the foot print, will result in error
	std::vector< std::pair<int,double> > parameters;
	double fsd = spline->findFootPrint( points, parameters);
	int iterNum = 0;
	while( fsd > eplison && iterNum < maxIterNum)
	{
		MatrixXd ehm(2*controlNum, 2*controlNum);
		VectorXd ehv( 2*controlNum);

		ehm.setZero();
		ehv.setZero();

		// compute h(D)
		for( int i = 0; i< (int)parameters.size(); i++)
		{
			spline->getDistance_sd( points[i],parameters[i], ehm, ehv );	
		}
		// check if ehm, ehv right
		//solve the function
		MatrixXd fm = ehm*0.5 + pm*alpha + sm*beta; 
		VectorXd ehv2 = ehv*0.5;
		JacobiSVD<MatrixXd> svd(fm, ComputeThinU | ComputeThinV);
		VectorXd resultxy = svd.solve(ehv2);

		// update the curve
		for( int i = 0; i<controlNum; i++)
			controlPs[i] = Vector2d( resultxy[i], resultxy[i+controlNum]);
		spline->setNewControl( controlPs );	
		++ iterNum;

		fsd = spline->findFootPrint( points, parameters );
	}

	return fsd;


}
