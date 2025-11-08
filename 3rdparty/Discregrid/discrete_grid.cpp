#include <discrete_grid.hpp>

using namespace Eigen;

namespace Discregrid
{


DiscreteGrid::MultiIndex
DiscreteGrid::singleToMultiIndex(unsigned int l) const
{
	auto n01 = m_resolution[0] * m_resolution[1];
	auto k = l / n01;
	auto temp = l % n01;
	auto j = temp / m_resolution[0];
	auto i = temp % m_resolution[0];
	return {{i, j, k}};
}

unsigned int
DiscreteGrid::multiToSingleIndex(MultiIndex const & ijk) const
{
	return m_resolution[1] * m_resolution[0] * ijk[2] + m_resolution[0] * ijk[1] + ijk[0];
}

AlignedBox3d
DiscreteGrid::subdomain(MultiIndex const& ijk) const
{
	auto origin = m_domain.min() + Map<Matrix<unsigned int, 3, 1> const>(
		ijk.data()).cast<double>().cwiseProduct(m_cell_size);
	return { origin, origin + m_cell_size};
}

AlignedBox3d
DiscreteGrid::subdomain(unsigned int l) const
{
	return subdomain(singleToMultiIndex(l));
}


}