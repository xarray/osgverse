#pragma once


#include <Eigen/Dense>

class CubicKernel
{
public:
	 double getRadius() { return m_radius; }
	 void setRadius(double val)
	{
		m_radius = val;
		const double pi = static_cast<double>(M_PI);

		const double h3 = m_radius*m_radius*m_radius;
		m_k = 8.0 / (pi*h3);
		m_l = 48.0 / (pi*h3);
		m_W_zero = W(Eigen::Vector3d::Zero());
	}

public:
	double W(Eigen::Vector3d const& r)
	{
		double res = 0.0;
		const double rl = r.norm();
		const double q = rl/m_radius;
		if (q <= 1.0)
		{
			if (q <= 0.5)
			{
				const double q2 = q*q;
				const double q3 = q2*q;
				res = m_k * (6.0*q3-6.0*q2+1.0);
			}
			else
			{
				auto _1mq = 1.0 - q;
				res = m_k * (2.0*_1mq*_1mq*_1mq);
			}
		}
		return res;
	}

	Eigen::Vector3d gradW(const Eigen::Vector3d &r)
	{
		using namespace Eigen;
		Vector3d res;
		const double rl = r.norm();
		const double q = rl / m_radius;
		if (q <= 1.0)
		{
			if (rl > 1.0e-6)
			{
				const Vector3d gradq = r * ((double) 1.0 / (rl*m_radius));
				if (q <= 0.5)
				{
					res = m_l*q*((double) 3.0*q - (double) 2.0)*gradq;
				}
				else
				{
					const double factor = 1.0 - q;
					res = m_l*(-factor*factor)*gradq;
				}
			}
		}
		else
			res.setZero();

		return res;
	}

	double W_zero()
	{
		return m_W_zero;
	}

private:
	double m_radius;
	double m_k;
	double m_l;
	double m_W_zero;
};
