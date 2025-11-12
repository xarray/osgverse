#pragma once

#include <Eigen/Dense>

class GaussQuadrature
{
public:

    using Integrand = std::function<double(Eigen::Vector3d const&)>;
    using Domain = Eigen::AlignedBox3d;

    static double integrate(Integrand integrand, Domain const& domain, unsigned int p);
};



