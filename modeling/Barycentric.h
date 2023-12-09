#ifndef MANA_MODELING_BARYCENTRIC_HPP
#define MANA_MODELING_BARYCENTRIC_HPP

/// Copied and modified from igl/barycentric_coordinates.h
#include <Eigen/Core>

namespace osgVerse
{
    /// Compute barycentric coordinates in a triangle
    ///
    ///   @param[in] P  #P by dim Query points
    ///   @param[in] A  #P by dim Triangle corners
    ///   @param[in] B  #P by dim Triangle corners
    ///   @param[in] C  #P by dim Triangle corners
    ///   @param[out] L  #P by 3 list of barycentric coordinates
    template<
        typename DerivedP,
        typename DerivedA,
        typename DerivedB,
        typename DerivedC,
        typename DerivedL>
    void barycentricCoordinates(
        const Eigen::MatrixBase<DerivedP>& P,
        const Eigen::MatrixBase<DerivedA>& A,
        const Eigen::MatrixBase<DerivedB>& B,
        const Eigen::MatrixBase<DerivedC>& C,
        Eigen::PlainObjectBase<DerivedL>& L)
    {
#ifndef NDEBUG
        const int DIM = P.cols();
        assert(A.cols() == DIM && "corners must be in same dimension as query");
        assert(B.cols() == DIM && "corners must be in same dimension as query");
        assert(C.cols() == DIM && "corners must be in same dimension as query");
        assert(P.rows() == A.rows() && "Must have same number of queries as corners");
        assert(A.rows() == B.rows() && "Corners must be same size");
        assert(A.rows() == C.rows() && "Corners must be same size");
#endif

        // http://gamedev.stackexchange.com/a/23745
        typedef Eigen::Array<
            typename DerivedP::Scalar,
            DerivedP::RowsAtCompileTime,
            DerivedP::ColsAtCompileTime> ArrayS;
        typedef Eigen::Array<
            typename DerivedP::Scalar,
            DerivedP::RowsAtCompileTime, 1> VectorS;
        const ArrayS v0 = B.array() - A.array();
        const ArrayS v1 = C.array() - A.array();
        const ArrayS v2 = P.array() - A.array();
        VectorS d00 = (v0 * v0).rowwise().sum();
        VectorS d01 = (v0 * v1).rowwise().sum();
        VectorS d11 = (v1 * v1).rowwise().sum();
        VectorS d20 = (v2 * v0).rowwise().sum();
        VectorS d21 = (v2 * v1).rowwise().sum();
        VectorS denom = d00 * d11 - d01 * d01;
        L.resize(P.rows(), 3);
        L.col(1) = (d11 * d20 - d01 * d21) / denom;
        L.col(2) = (d00 * d21 - d01 * d20) / denom;
        L.col(0) = 1.0f - (L.col(1) + L.col(2)).array();
    }

    // Explicit template instantiation
    template void barycentricCoordinates<
        Eigen::Matrix<float, 1, -1, 1, 1, -1>, Eigen::Matrix<float, 1, 3, 1, 1, 3>,
        Eigen::Matrix<float, 1, 3, 1, 1, 3>, Eigen::Matrix<float, 1, 3, 1, 1, 3>,
        Eigen::Matrix<float, 1, 3, 1, 1, 3> > (
            Eigen::MatrixBase<Eigen::Matrix<float, 1, -1, 1, 1, -1> > const&,
            Eigen::MatrixBase<Eigen::Matrix<float, 1, 3, 1, 1, 3> > const&,
            Eigen::MatrixBase<Eigen::Matrix<float, 1, 3, 1, 1, 3> > const&,
            Eigen::MatrixBase<Eigen::Matrix<float, 1, 3, 1, 1, 3> > const&,
            Eigen::PlainObjectBase<Eigen::Matrix<float, 1, 3, 1, 1, 3> >&);
    template void barycentricCoordinates<
        Eigen::Matrix<double, 1, -1, 1, 1, -1>, Eigen::Matrix<double, 1, 3, 1, 1, 3>,
        Eigen::Matrix<double, 1, 3, 1, 1, 3>, Eigen::Matrix<double, 1, 3, 1, 1, 3>,
        Eigen::Matrix<double, 1, 3, 1, 1, 3> > (
            Eigen::MatrixBase<Eigen::Matrix<double, 1, -1, 1, 1, -1> > const&,
            Eigen::MatrixBase<Eigen::Matrix<double, 1, 3, 1, 1, 3> > const&,
            Eigen::MatrixBase<Eigen::Matrix<double, 1, 3, 1, 1, 3> > const&,
            Eigen::MatrixBase<Eigen::Matrix<double, 1, 3, 1, 1, 3> > const&,
            Eigen::PlainObjectBase<Eigen::Matrix<double, 1, 3, 1, 1, 3> >&);
    template void barycentricCoordinates<
        Eigen::Matrix<float, 1, 3, 1, 1, 3>, Eigen::Matrix<float, 1, 3, 1, 1, 3>,
        Eigen::Matrix<float, 1, 3, 1, 1, 3>, Eigen::Matrix<float, 1, 3, 1, 1, 3>,
        Eigen::Matrix<float, 1, 3, 1, 1, 3> > (
            Eigen::MatrixBase<Eigen::Matrix<float, 1, 3, 1, 1, 3> > const&,
            Eigen::MatrixBase<Eigen::Matrix<float, 1, 3, 1, 1, 3> > const&,
            Eigen::MatrixBase<Eigen::Matrix<float, 1, 3, 1, 1, 3> > const&,
            Eigen::MatrixBase<Eigen::Matrix<float, 1, 3, 1, 1, 3> > const&,
            Eigen::PlainObjectBase<Eigen::Matrix<float, 1, 3, 1, 1, 3> >&);
    template void barycentricCoordinates<
        Eigen::Matrix<double, 1, 3, 1, 1, 3>, Eigen::Matrix<double, 1, 3, 1, 1, 3>,
        Eigen::Matrix<double, 1, 3, 1, 1, 3>, Eigen::Matrix<double, 1, 3, 1, 1, 3>,
        Eigen::Matrix<double, 1, 3, 1, 1, 3> > (
            Eigen::MatrixBase<Eigen::Matrix<double, 1, 3, 1, 1, 3> > const&,
            Eigen::MatrixBase<Eigen::Matrix<double, 1, 3, 1, 1, 3> > const&,
            Eigen::MatrixBase<Eigen::Matrix<double, 1, 3, 1, 1, 3> > const&,
            Eigen::MatrixBase<Eigen::Matrix<double, 1, 3, 1, 1, 3> > const&,
            Eigen::PlainObjectBase<Eigen::Matrix<double, 1, 3, 1, 1, 3> >&);
    template void barycentricCoordinates<
        Eigen::Matrix<double, 1, 2, 1, 1, 2>,
        Eigen::Block<Eigen::Matrix<double, -1, -1, 0, -1, -1> const, 1, -1, false>,
        Eigen::Block<Eigen::Matrix<double, -1, -1, 0, -1, -1> const, 1, -1, false>,
        Eigen::Block<Eigen::Matrix<double, -1, -1, 0, -1, -1> const, 1, -1, false>,
        Eigen::Matrix<double, 1, 3, 1, 1, 3> > (
            Eigen::MatrixBase<Eigen::Matrix<double, 1, 2, 1, 1, 2> > const&,
            Eigen::MatrixBase<Eigen::Block<Eigen::Matrix<double, -1, -1, 0, -1, -1> const, 1, -1, false> > const&,
            Eigen::MatrixBase<Eigen::Block<Eigen::Matrix<double, -1, -1, 0, -1, -1> const, 1, -1, false> > const&,
            Eigen::MatrixBase<Eigen::Block<Eigen::Matrix<double, -1, -1, 0, -1, -1> const, 1, -1, false> > const&,
            Eigen::PlainObjectBase<Eigen::Matrix<double, 1, 3, 1, 1, 3> >&);
    template void barycentricCoordinates<
        Eigen::Matrix<double, 1, 3, 1, 1, 3>,
        Eigen::Block<Eigen::Matrix<double, -1, -1, 0, -1, -1> const, 1, -1, false>,
        Eigen::Block<Eigen::Matrix<double, -1, -1, 0, -1, -1> const, 1, -1, false>,
        Eigen::Block<Eigen::Matrix<double, -1, -1, 0, -1, -1> const, 1, -1, false>,
        Eigen::Matrix<double, 1, 3, 1, 1, 3> > (
            Eigen::MatrixBase<Eigen::Matrix<double, 1, 3, 1, 1, 3> > const&,
            Eigen::MatrixBase<Eigen::Block<Eigen::Matrix<double, -1, -1, 0, -1, -1> const, 1, -1, false> > const&,
            Eigen::MatrixBase<Eigen::Block<Eigen::Matrix<double, -1, -1, 0, -1, -1> const, 1, -1, false> > const&,
            Eigen::MatrixBase<Eigen::Block<Eigen::Matrix<double, -1, -1, 0, -1, -1> const, 1, -1, false> > const&,
            Eigen::PlainObjectBase<Eigen::Matrix<double, 1, 3, 1, 1, 3> >&);
    template void barycentricCoordinates<
        Eigen::Matrix<double, -1, -1, 0, -1, -1>, Eigen::Matrix<double, -1, -1, 0, -1, -1>,
        Eigen::Matrix<double, -1, -1, 0, -1, -1>, Eigen::Matrix<double, -1, -1, 0, -1, -1>,
        Eigen::Matrix<double, -1, -1, 0, -1, -1> > (
            Eigen::MatrixBase<Eigen::Matrix<double, -1, -1, 0, -1, -1> > const&,
            Eigen::MatrixBase<Eigen::Matrix<double, -1, -1, 0, -1, -1> > const&,
            Eigen::MatrixBase<Eigen::Matrix<double, -1, -1, 0, -1, -1> > const&,
            Eigen::MatrixBase<Eigen::Matrix<double, -1, -1, 0, -1, -1> > const&,
            Eigen::PlainObjectBase<Eigen::Matrix<double, -1, -1, 0, -1, -1> >&);
}

#endif
