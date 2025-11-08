#pragma once

#include "discrete_grid.hpp"

namespace Discregrid
{

class CubicLagrangeDiscreteGrid : public DiscreteGrid
{
public:

	CubicLagrangeDiscreteGrid(std::string const& filename);
	CubicLagrangeDiscreteGrid(Eigen::AlignedBox3d const& domain,
		std::array<unsigned int, 3> const& resolution);

	void save(std::string const& filename) const override;
	void load(std::string const& filename) override;

	unsigned int addFunction(ContinuousFunction const& func, bool verbose = false,
		SamplePredicate const& pred = nullptr) override;


	std::size_t nCells() const { return m_n_cells; };
	double interpolate(unsigned int field_id, Eigen::Vector3d const& xi,
		Eigen::Vector3d* gradient = nullptr) const override;

	/**
	 * @brief Determines the shape functions for the discretization with ID field_id at point xi.
	 * 
	 * @param field_id Discretization ID
	 * @param x Location where the shape functions should be determined
	 * @param cell cell of x
	 * @param c0 vector required for the interpolation
	 * @param N	shape functions for the cell of x
	 * @param dN (Optional) derivatives of the shape functions, required to compute the gradient
	 * @return Success of the function.
	 */
	bool determineShapeFunctions(unsigned int field_id, Eigen::Vector3d const &x,
		std::array<unsigned int, 32> &cell, Eigen::Vector3d &c0, Eigen::Matrix<double, 32, 1> &N, 
		Eigen::Matrix<double, 32, 3> *dN = nullptr) const override;

	/**
	 * @brief Evaluates the given discretization with ID field_id at point xi.
	 * 
	 * @param field_id Discretization ID
	 * @param xi Location where the discrete function is evaluated
	 * @param cell cell of xi
	 * @param c0 vector required for the interpolation
	 * @param N	shape functions for the cell of xi
	 * @param gradient (Optional) if a pointer to a vector is passed the gradient of the discrete function will be evaluated
	 * @param dN (Optional) derivatives of the shape functions, required to compute the gradient
	 * @return double Results of the evaluation of the discrete function at point xi
	 */
	double interpolate(unsigned int field_id, Eigen::Vector3d const& xi, const std::array<unsigned int, 32> &cell, const Eigen::Vector3d &c0, const Eigen::Matrix<double, 32, 1> &N,
		Eigen::Vector3d* gradient = nullptr, Eigen::Matrix<double, 32, 3> *dN = nullptr) const override;

	void reduceField(unsigned int field_id, Predicate pred) override;

	void forEachCell(unsigned int field_id,
		std::function<void(unsigned int, Eigen::AlignedBox3d const&, unsigned int)> const& cb) const;

private:

	Eigen::Vector3d indexToNodePosition(unsigned int l) const;


private:

	std::vector<std::vector<double>> m_nodes;
	std::vector<std::vector<std::array<unsigned int, 32>>> m_cells;
	std::vector<std::vector<unsigned int>> m_cell_map;
};

}