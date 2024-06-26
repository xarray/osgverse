/// lsqcpp/lsqcpp.hpp
///
/// Author:     Fabian Meyer
/// Created On: 22 Jul 2019
/// License:    MIT

#ifndef LSQCPP_LSQCPP_HPP_
#define LSQCPP_LSQCPP_HPP_

#include <Eigen/Core>
#include <Eigen/SVD>
#include <Eigen/Cholesky>
#include <iostream>
#include <iomanip>
#include <functional>

namespace lsqcpp
{
    using Index = Eigen::MatrixXd::Index;

    /// Parametrization container for jacobian estimation using finite differences.
    /// This class holds parameters for the finite differences operation, such as the
    /// number of threads to be used and the numerical espilon.
    template<typename _Scalar>
    struct FiniteDifferencesParameter
    {
    public:
        using Scalar = _Scalar;

        static_assert(Eigen::NumTraits<Scalar>::IsInteger == 0, "Finite differences only supports non-integer scalars.");

        FiniteDifferencesParameter() = default;

        FiniteDifferencesParameter(const Scalar eps)
            : _eps(eps)
        { }

        FiniteDifferencesParameter(const Scalar eps, const int threads)
            : _eps(eps), _threads(threads)
        { }

        /// Sets the numerical espilon that is used as step between two sucessive function evaluations.
        /// @param eps numerical epsilon
        void setEpsilon(const Scalar eps)
        {
            _eps = eps;
        }

        /// Returns the numerical epsilon, which is used for jacobian approximation.
        /// @return numerical epsilon
        Scalar epsilon() const
        {
            return _eps;
        }

        /// Sets the number of threads which should be used to compute finite differences
        /// (OMP only).
        /// Set to 0 or negative for auto-detection of a suitable number.
        /// Each dimension of the input vector can be handled independently.
        /// @param threads number of threads
        void setThreads(const int threads)
        {
            _threads = threads;
        }

        /// Returns the number of threads which should be used to compute finite differences
        /// (OMP only).
        /// @return threads number of threads
        int threads() const
        {
            return _threads;
        }

    private:
        Scalar _eps = std::sqrt(Eigen::NumTraits<Scalar>::epsilon());
        int _threads = int{1};
    };

    /// Functor to compute forward differences.
    /// Computes the gradient of the objective f(x) as follows:
    ///
    /// grad(x) = (f(x + eps) - f(x)) / eps
    ///
    /// The computation requires len(x) evaluations of the objective.
    struct ForwardDifferences
    {
        template<typename Scalar, typename Objective, typename I, typename O, typename J>
        void operator()(const Eigen::MatrixBase<I> &xval,
                        const Eigen::MatrixBase<O> &fval,
                        const Objective &objective,
                        const FiniteDifferencesParameter<Scalar>& param,
                        Eigen::MatrixBase<J> &jacobian) const
        {
            static_assert(Eigen::NumTraits<Scalar>::IsInteger == 0, "Finite differences only supports non-integer scalars.");

            using InputVector = typename Eigen::MatrixBase<I>::PlainMatrix;
            using OutputVector = typename Eigen::MatrixBase<O>::PlainMatrix;

            // noop in fixed size problems
            jacobian.derived().resize(fval.size(), xval.size());

            #pragma omp parallel for num_threads(param.threads())
            for(Index i = 0; i < xval.size(); ++i)
            {
                InputVector xvalNext = xval;
                OutputVector fvalNext;
                xvalNext(i) += param.epsilon();
                objective(xvalNext, fvalNext);

                jacobian.col(i) = (fvalNext - fval) / param.epsilon();
            }
        }
    };

    /// Functor to compute backward differences.
    /// Computes the gradient of the objective f(x) as follows:
    ///
    /// grad(x) = (f(x) - f(x - eps)) / eps
    ///
    /// The computation requires len(x) evaluations of the objective.
    struct BackwardDifferences
    {
        template<typename Scalar, typename Objective, typename I, typename O, typename J>
        void operator()(const Eigen::MatrixBase<I> &xval,
                        const Eigen::MatrixBase<O> &fval,
                        const Objective &objective,
                        const FiniteDifferencesParameter<Scalar>& param,
                        Eigen::MatrixBase<J> &jacobian) const
        {
            static_assert(Eigen::NumTraits<Scalar>::IsInteger == 0, "Finite differences only supports non-integer scalars.");

            using InputVector = typename Eigen::MatrixBase<I>::PlainMatrix;
            using OutputVector = typename Eigen::MatrixBase<O>::PlainMatrix;

            // noop in fixed size problems
            jacobian.derived().resize(fval.size(), xval.size());

            #pragma omp parallel for num_threads(param.threads())
            for(Index i = 0; i < xval.size(); ++i)
            {
                InputVector xvalNext = xval;
                OutputVector fvalNext;
                xvalNext(i) -= param.epsilon();
                objective(xvalNext, fvalNext);

                jacobian.col(i) = (fval - fvalNext) / param.epsilon();
            }
        }
    };

    /// Functor to compute central differences.
    /// Computes the gradient of the objective f(x) as follows:
    ///
    /// grad(x) = (f(x + 0.5 eps) - f(x - 0.5 eps)) / eps
    ///
    /// The computation requires 2 * len(x) evaluations of the objective.
    struct CentralDifferences
    {
        template<typename Scalar, typename Objective, typename I, typename O, typename J>
        void operator()(const Eigen::MatrixBase<I> &xval,
                        const Eigen::MatrixBase<O> &fval,
                        const Objective &objective,
                        const FiniteDifferencesParameter<Scalar>& param,
                        Eigen::MatrixBase<J> &jacobian) const
        {
            static_assert(Eigen::NumTraits<Scalar>::IsInteger == 0, "Finite differences only supports non-integer scalars.");

            using InputVector = typename Eigen::MatrixBase<I>::PlainMatrix;
            using OutputVector = typename Eigen::MatrixBase<O>::PlainMatrix;

            // noop in fixed size problems
            jacobian.derived().resize(fval.size(), xval.size());

            #pragma omp parallel for num_threads(param.threads())
            for(Index i = 0; i < xval.size(); ++i)
            {
                InputVector xvalNext = xval;
                OutputVector fvalA;
                OutputVector fvalB;

                xvalNext(i) = xval(i) - param.epsilon() / Scalar{2};
                objective(xvalNext, fvalA);

                xvalNext(i) = xval(i) + param.epsilon() / Scalar{2};
                objective(xvalNext, fvalB);

                jacobian.col(i) = (fvalB - fvalA) / param.epsilon();
            }
        }
    };

    /// Solves for dense linear equation systems using the Jacobi SVD method.
    struct DenseSVDSolver
    {
        /// Solves the linear equation system Ax = b and stores the result in x.
        /// @param A linear equation system matrix
        /// @param b linear equation system vector
        /// @param x computed result
        /// @return true if successful, false otherwise
        template<typename M, typename B, typename X>
        bool operator()(const Eigen::MatrixBase<M> &A,
                        const Eigen::MatrixBase<B> &b,
                        Eigen::MatrixBase<X> &x) const
        {
            using Solver = Eigen::JacobiSVD<M, Eigen::FullPivHouseholderQRPreconditioner>;
            auto solver = Solver(A, Eigen::ComputeFullU | Eigen::ComputeFullV);
            x = solver.solve(b);
            return true;
        }
    };

    /// Solver for dense linear equation systems, which a positive semi-definite, using the Cholesky decomposition.
    struct DenseCholeskySolver
    {
        /// Solves the linear equation system Ax = b and stores the result in x.
        /// @param A linear equation system matrix
        /// @param b linear equation system vector
        /// @param x computed result
        /// @return true if successful, false otherwise
        template<typename M, typename B, typename X>
        bool operator()(const Eigen::MatrixBase<M> &A,
                        const Eigen::MatrixBase<B> &b,
                        Eigen::MatrixBase<X> &x) const
        {
            using Solver = Eigen::LDLT<M>;

            Solver decomp;
            decomp.compute(A);

            if(!decomp.isPositive())
                return false;

            x = decomp.solve(b);
            return true;
        }
    };

    /// Generic class for refining a computed newton step.
    /// The method parameter determines how the step is actually refined, e.g
    /// ArmijoBacktracking, DoglegMethod or WolfeBacktracking.
    template<typename Scalar, int Inputs, int Outputs, typename Method>
    class NewtonStepRefiner { };

    /// Newton step refinement method which applies a constant scaling factor to the newton step.
    struct ConstantStepFactor { };

    /// Parametrization class for the ConstantStepFactor refinement method.
    template<typename _Scalar>
    class ConstantStepFactorParameter
    {
    public:
        using Scalar = _Scalar;

        ConstantStepFactorParameter() = default;

        ConstantStepFactorParameter(const Scalar factor)
            : _factor(factor)
        { }
        /// Sets the constant scaling factor which is applied to the newton step.
        /// @param factor constant newton step scaling factor
        void setFactor(const Scalar factor)
        {
            _factor = factor;
        }

        /// Returns the constant scaling factor which is applied to the newton step.
        /// @return constant newton step scaling factor
        Scalar factor() const
        {
            return _factor;
        }
    private:
        Scalar _factor = Scalar{1};
    };

    template<typename _Scalar, int _Inputs, int _Outputs>
    class NewtonStepRefiner<_Scalar, _Inputs, _Outputs, ConstantStepFactor>
    {
    public:
        using Scalar = _Scalar;
        static constexpr int Inputs = _Inputs;
        static constexpr int Outputs = _Outputs;
        using Method = ConstantStepFactor;
        using Parameter = ConstantStepFactorParameter<Scalar>;

        static_assert(Eigen::NumTraits<Scalar>::IsInteger == 0, "Step refinement only supports non-integer scalars");

        using InputVector = Eigen::Matrix<Scalar, Inputs, 1>;
        using OutputVector = Eigen::Matrix<Scalar, Outputs, 1>;
        using JacobiMatrix = Eigen::Matrix<Scalar, Outputs, Inputs>;
        using GradientVector = Eigen::Matrix<Scalar, Inputs, 1>;
        using StepVector = Eigen::Matrix<Scalar, Inputs, 1>;

        NewtonStepRefiner(const Parameter &param)
            : _param(param)
        { }

        /// Refines the given newton step and scales it by a constant factor.
        /// @param step newton step which is scaled.
        template<typename Objective>
        void operator()(const InputVector &,
                        const OutputVector &,
                        const JacobiMatrix &,
                        const GradientVector &,
                        const Objective &,
                        StepVector &step) const
        {
            step *= _param.factor();
        }
    private:
        const Parameter &_param;
    };

    /// Applies Barzilai-Borwein (BB) refinemnt to the newton step.
    /// The functor can either compute the direct or inverse BB step.
    /// The steps are computed as follows:
    ///
    /// s_k = x_k - x_k-1         k >= 1
    /// y_k = step_k - step_k-1   k >= 1
    /// Direct:  stepSize = (s_k^T * s_k) / (y_k^T * s_k)
    /// Inverse: stepSize = (y_k^T * s_k) / (y_k^T * y_k)
    ///
    /// The very first step is computed as a constant.
    struct BarzilaiBorwein
    {
        enum class Mode
        {
            Direct,
            Inverse
        };
    };

    /// Parametrization class for the BarzilaiBorwein refinement method.
    template<typename _Scalar>
    class BarzilaiBorweinParameter
    {
    public:
        using Scalar = _Scalar;

        BarzilaiBorweinParameter() = default;

        BarzilaiBorweinParameter(const BarzilaiBorwein::Mode mode)
            : _mode(mode)
        { }

        BarzilaiBorweinParameter(const Scalar constStep)
            : _constStep(constStep)
        { }

        BarzilaiBorweinParameter(const BarzilaiBorwein::Mode mode, const Scalar constStep)
            : _mode(mode), _constStep(constStep)
        { }

        /// Sets the BarzilaiBorwein operation mode.
        /// @param mode mode
        void setMode(const BarzilaiBorwein::Mode mode)
        {
            _mode = mode;
        }

        /// Returns the BarzilaiBorwein operation mode.
        /// @return mode
        BarzilaiBorwein::Mode mode() const
        {
            return _mode;
        }

        /// Sets the constant step size, which is used when the refiner was not initialized yet.
        /// @param stepSize constant step size
        void setConstantStepSize(const Scalar stepSize)
        {
            _constStep = stepSize;
        }

        /// Returns he constant step size, which is used when the refiner was not initialized yet.
        /// @return constant step size
        Scalar constantStepSize() const
        {
            return _constStep;
        }
    private:
        BarzilaiBorwein::Mode _mode = BarzilaiBorwein::Mode::Direct;
        Scalar _constStep = static_cast<Scalar>(1e-2);
    };

    template<typename _Scalar, int _Inputs, int _Outputs>
    class NewtonStepRefiner<_Scalar, _Inputs, _Outputs, BarzilaiBorwein>
    {
    public:
        using Scalar = _Scalar;
        static constexpr int Inputs = _Inputs;
        static constexpr int Outputs = _Outputs;
        using Method = BarzilaiBorwein;
        using Parameter = BarzilaiBorweinParameter<Scalar>;

        static_assert(Eigen::NumTraits<Scalar>::IsInteger == 0, "Step refinement only supports non-integer scalars");

        using InputVector = Eigen::Matrix<Scalar, Inputs, 1>;
        using OutputVector = Eigen::Matrix<Scalar, Outputs, 1>;
        using JacobiMatrix = Eigen::Matrix<Scalar, Outputs, Inputs>;
        using GradientVector = Eigen::Matrix<Scalar, Inputs, 1>;
        using StepVector = Eigen::Matrix<Scalar, Inputs, 1>;

        NewtonStepRefiner(const Parameter &param)
            : _param(param)
        {
            _lastXval.setZero();
            _lastStep.setZero();
        }

        template<typename Objective>
        void operator()(const InputVector &xval,
            const OutputVector &,
            const JacobiMatrix &,
            const GradientVector &,
            const Objective &,
            StepVector &step)
        {
            auto stepSize = Scalar{0};
            if(_lastXval.sum() == Scalar{0})
            {
                stepSize = (Scalar{1} / step.norm()) * _param.constantStepSize();
            }
            else
            {
                switch(_param.mode())
                {
                case BarzilaiBorwein::Mode::Direct:
                    stepSize = directStep(xval, step);
                    break;
                case BarzilaiBorwein::Mode::Inverse:
                    stepSize = inverseStep(xval, step);
                    break;
                default:
                    assert(false);
                    break;
                }
            }

            _lastStep = step;
            _lastXval = xval;

            step *= stepSize;
        }
    private:
        const Parameter &_param;
        InputVector _lastXval = {};
        StepVector _lastStep = {};

        Scalar directStep(const InputVector &xval,
                          const StepVector &step) const
        {
            const auto sk = xval - _lastXval;
            const auto yk = step - _lastStep;
            const auto num = sk.dot(sk);
            const auto denom = sk.dot(yk);

            if(denom == Scalar{0})
                return Scalar{1};
            else
                return num / denom;
        }

        Scalar inverseStep(const InputVector &xval,
                           const StepVector &step) const
        {
            const auto sk = xval - _lastXval;
            const auto yk = step - _lastStep;
            const auto num = sk.dot(yk);
            const auto denom = yk.dot(yk);

            if(denom == Scalar{0})
                return Scalar{1};
            else
                return num / denom;
        }
    };

    /// Step refinement method performs Armijo Linesearch with backtracking.
    /// The line search iteratively decreases the step size until the following
    /// conditions are met:
    ///
    /// Armijo: f(x - stepSize * grad(x)) <= f(x) - c1 * stepSize * grad(x)^T * grad(x)
    ///
    /// If the condition does not hold the step size is decreased:
    ///
    /// stepSize = decrease * stepSize
    struct ArmijoBacktracking { };

    /// Parametrization class for the ArmijoBacktracking refinement method.
    template<typename _Scalar>
    class ArmijoBacktrackingParameter
    {
    public:
        using Scalar = _Scalar;

        ArmijoBacktrackingParameter() = default;

        ArmijoBacktrackingParameter(const Scalar decrease,
                                    const Scalar c1,
                                    const Scalar minStep,
                                    const Scalar maxStep,
                                    const Index iterations)
            : _decrease(decrease), _c1(c1), _minStep(minStep),
            _maxStep(maxStep), _maxIt(iterations)
        {
            assert(decrease > static_cast<Scalar>(0));
            assert(decrease < static_cast<Scalar>(1));
            assert(c1 > static_cast<Scalar>(0));
            assert(c1 < static_cast<Scalar>(0.5));
        }

        /// Set the decreasing factor for backtracking.
        /// Assure that decrease in (0, 1).
        /// @param decrease decreasing factor
        void setBacktrackingDecrease(const Scalar decrease)
        {
            assert(decrease > static_cast<Scalar>(0));
            assert(decrease < static_cast<Scalar>(1));
            _decrease = decrease;
        }

        /// Returns the decreasing factor for backtracking.
        /// The value should always lie within (0, 1).
        /// @return backtracking decrease
        Scalar backtrackingDecrease() const
        {
            return _decrease;
        }

        /// Set the relaxation constant for the Armijo condition (see class description).
        /// Typically c1 is chosen to be quite small, e.g. 1e-4.
        /// Assure that c1 in (0, 0.5).
        /// @param c1 armijo constant
        void setArmijoConstant(const Scalar c1)
        {
            assert(c1 > static_cast<Scalar>(0));
            assert(c1 < static_cast<Scalar>(0.5));
            _c1 = c1;
        }

        /// Returns the the relaxation constant for the Armijo condition (see class description).
        /// The value should always lie within (0, 0.5).
        /// @return armijo constant
        Scalar armijoConstant() const
        {
            return _c1;
        }

        /// Set the bounds for the step size during linesearch.
        /// The final step size is guaranteed to be in [minStep, maxStep].
        /// @param minStep minimum step size
        /// @param maxStep maximum step size
        void setStepBounds(const Scalar minStep, const Scalar maxStep)
        {
            assert(minStep >= 0);
            assert(minStep < maxStep);
            _minStep = minStep;
            _maxStep = maxStep;
        }

        /// Returns the minimum bound for the step size during linesearch.
        /// @return minimum step size bound
        Scalar minimumStepBound() const
        {
            return _minStep;
        }

        /// Returns the maximum bound for the step size during linesearch.
        /// @return maximum step size bound
        Scalar maximumStepBound() const
        {
            return _maxStep;
        }

        /// Set the maximum number of iterations.
        /// Set to 0 or negative for infinite iterations.
        /// @param iterations maximum number of iterations
        void setMaximumIterations(const Index iterations)
        {
            _maxIt = iterations;
        }

        /// Returns the maximum number of iterations.
        /// A value of 0 or negative means infinite iterations.
        /// @return maximum number of iterations
        Index maximumIterations() const
        {
            return _maxIt;
        }
    private:
        Scalar _decrease = static_cast<Scalar>(0.8);
        Scalar _c1 = static_cast<Scalar>(1e-4);
        Scalar _minStep = static_cast<Scalar>(1e-10);
        Scalar _maxStep = static_cast<Scalar>(1);
        Index _maxIt = Index{0};
    };

    template<typename _Scalar, int _Inputs, int _Outputs>
    class NewtonStepRefiner<_Scalar, _Inputs, _Outputs, ArmijoBacktracking>
    {
    public:
        using Scalar = _Scalar;
        static constexpr int Inputs = _Inputs;
        static constexpr int Outputs = _Outputs;
        using Method = ArmijoBacktracking;
        using Parameter = ArmijoBacktrackingParameter<Scalar>;

        static_assert(Eigen::NumTraits<Scalar>::IsInteger == 0, "Step refinement only supports non-integer scalars");

        using InputVector = Eigen::Matrix<Scalar, Inputs, 1>;
        using OutputVector = Eigen::Matrix<Scalar, Outputs, 1>;
        using JacobiMatrix = Eigen::Matrix<Scalar, Outputs, Inputs>;
        using GradientVector = Eigen::Matrix<Scalar, Inputs, 1>;
        using StepVector = Eigen::Matrix<Scalar, Inputs, 1>;

        NewtonStepRefiner(const Parameter &param)
            : _param(param)
        { }

        template<typename Objective>
        void operator()(const InputVector &xval,
                        const OutputVector &fval,
                        const JacobiMatrix &,
                        const GradientVector &gradient,
                        const Objective &objective,
                        StepVector &step) const
        {
            JacobiMatrix jacobianN;
            InputVector xvalN;
            OutputVector fvalN;

            const auto error = static_cast<Scalar>(0.5) * fval.squaredNorm();
            const auto stepGrad = gradient.dot(step);
            bool armijoCondition = false;
            auto stepSize = _param.maximumStepBound() / _param.backtrackingDecrease();

            auto iterations = Index{0};
            while((_param.maximumIterations() <= Index{0} || iterations < _param.maximumIterations()) &&
                   stepSize * _param.backtrackingDecrease() >= _param.minimumStepBound() &&
                   !armijoCondition)
            {
                stepSize = _param.backtrackingDecrease() * stepSize;
                xvalN = xval - stepSize * step;
                objective(xvalN, fvalN, jacobianN);

                const auto errorN = static_cast<Scalar>(0.5) * fvalN.squaredNorm();

                armijoCondition = errorN <= error + _param.armijoConstant() * stepSize * stepGrad;

                ++iterations;
            }

            step *= stepSize;
        }
    private:
        const Parameter &_param;
    };

    /// Step refinement method which performs Wolfe Linesearch with backtracking.
    /// The line search iteratively decreases the step size until the following
    /// conditions are met:
    ///
    /// Armijo: f(x - stepSize * grad(x)) <= f(x) - c1 * stepSize * grad(x)^T * grad(x)
    /// Wolfe: grad(x)^T grad(x - stepSize * grad(x)) <= c2 * grad(x)^T * grad(x)
    ///
    /// If either condition does not hold the step size is decreased:
    ///
    /// stepSize = decrease * stepSize
    struct WolfeBacktracking { };

    /// Parametrization class for the WolfeBacktracking refinement method.
    template<typename _Scalar>
    class WolfeBacktrackingParameter
    {
    public:
        using Scalar = _Scalar;

        WolfeBacktrackingParameter() = default;

        WolfeBacktrackingParameter(const Scalar decrease,
                                   const Scalar c1,
                                   const Scalar c2,
                                   const Scalar minStep,
                                   const Scalar maxStep,
                                   const Index iterations)
            : _decrease(decrease), _c1(c1), _c2(c2), _minStep(minStep),
            _maxStep(maxStep), _maxIt(iterations)
        {
            assert(decrease > static_cast<Scalar>(0));
            assert(decrease < static_cast<Scalar>(1));
            assert(c1 > static_cast<Scalar>(0));
            assert(c1 < static_cast<Scalar>(0.5));
            assert(c1 < c2);
            assert(c2 < static_cast<Scalar>(1));
        }

        /// Set the decreasing factor for backtracking.
        /// Assure that decrease in (0, 1).
        /// @param decrease decreasing factor
        void setBacktrackingDecrease(const Scalar decrease)
        {
            assert(decrease > static_cast<Scalar>(0));
            assert(decrease < static_cast<Scalar>(1));
            _decrease = decrease;
        }

        /// Returns the decreasing factor for backtracking.
        /// The value should always lie within (0, 1).
        /// @return backtracking decrease
        Scalar backtrackingDecrease() const
        {
            return _decrease;
        }

        /// Set the wolfe constants for Armijo and Wolfe condition (see class
        /// description).
        /// Assure that c1 < c2 < 1 and c1 in (0, 0.5).
        /// Typically c1 is chosen to be quite small, e.g. 1e-4.
        /// @param c1 armijo constant
        /// @param c2 wolfe constant
        void setWolfeConstants(const Scalar c1, const Scalar c2)
        {
            assert(c1 > static_cast<Scalar>(0));
            assert(c1 < static_cast<Scalar>(0.5));
            assert(c1 < c2);
            assert(c2 < static_cast<Scalar>(1));
            _c1 = c1;
            _c2 = c2;
        }

        /// Returns the the relaxation constant for the Armijo condition (see class description).
        /// The value should always lie within (0, 0.5).
        /// @return armijo constant
        Scalar armijoConstant() const
        {
            return _c1;
        }

        /// Returns the the relaxation constant for the Wolfe condition (see class description).
        /// @return wolfe constant
        Scalar wolfeConstant() const
        {
            return _c2;
        }

        /// Set the bounds for the step size during linesearch.
        /// The final step size is guaranteed to be in [minStep, maxStep].
        /// @param minStep minimum step size
        /// @param maxStep maximum step size
        void setStepBounds(const Scalar minStep, const Scalar maxStep)
        {
            assert(minStep >= Scalar{0});
            assert(minStep < maxStep);
            _minStep = minStep;
            _maxStep = maxStep;
        }

        /// Returns the minimum bound for the step size during linesearch.
        /// @return minimum step size bound
        Scalar minimumStepBound() const
        {
            return _minStep;
        }

        /// Returns the maximum bound for the step size during linesearch.
        /// @return maximum step size bound
        Scalar maximumStepBound() const
        {
            return _maxStep;
        }

        /// Set the maximum number of iterations.
        /// Set to 0 or negative for infinite iterations.
        /// @param iterations maximum number of iterations
        void setMaximumIterations(const Index iterations)
        {
            _maxIt = iterations;
        }

        /// Returns the maximum number of iterations.
        /// A value of 0 or negative means infinite iterations.
        /// @return maximum number of iterations
        Index maximumIterations() const
        {
            return _maxIt;
        }
    private:
        Scalar _decrease = static_cast<Scalar>(0.8);
        Scalar _c1 = static_cast<Scalar>(1e-4);
        Scalar _c2 = static_cast<Scalar>(0.9);
        Scalar _minStep = static_cast<Scalar>(1e-10);
        Scalar _maxStep = static_cast<Scalar>(1.0);
        Index _maxIt = 0;
    };

    template<typename _Scalar, int _Inputs, int _Outputs>
    class NewtonStepRefiner<_Scalar, _Inputs, _Outputs, WolfeBacktracking>
    {
    public:
        using Scalar = _Scalar;
        static constexpr int Inputs = _Inputs;
        static constexpr int Outputs = _Outputs;
        using Method = WolfeBacktracking;
        using Parameter = WolfeBacktrackingParameter<Scalar>;

        static_assert(Eigen::NumTraits<Scalar>::IsInteger == 0, "Step refinement only supports non-integer scalars");

        using InputVector = Eigen::Matrix<Scalar, Inputs, 1>;
        using OutputVector = Eigen::Matrix<Scalar, Outputs, 1>;
        using JacobiMatrix = Eigen::Matrix<Scalar, Outputs, Inputs>;
        using GradientVector = Eigen::Matrix<Scalar, Inputs, 1>;
        using StepVector = Eigen::Matrix<Scalar, Inputs, 1>;

        NewtonStepRefiner(const Parameter &param)
            : _param(param)
        {}

        template<typename Objective>
        void operator()(const InputVector &xval,
                        const OutputVector &fval,
                        const JacobiMatrix &,
                        const GradientVector &gradient,
                        const Objective &objective,
                        StepVector &step) const
        {
            auto stepSize = _param.maximumStepBound() / _param.backtrackingDecrease();
            JacobiMatrix jacobianN;
            GradientVector gradientN;
            InputVector xvalN;
            OutputVector fvalN;

            const Scalar error = fval.squaredNorm() / Scalar{2};
            const Scalar stepGrad = gradient.dot(step);
            bool armijoCondition = false;
            bool wolfeCondition = false;

            Index iterations = 0;
            while((_param.maximumIterations() <= 0 || iterations < _param.maximumIterations()) &&
                   stepSize * _param.backtrackingDecrease() >= _param.minimumStepBound() &&
                   !(armijoCondition && wolfeCondition))
            {
                stepSize = _param.backtrackingDecrease() * stepSize;
                xvalN = xval - stepSize * step;
                objective(xvalN, fvalN, jacobianN);
                Scalar errorN = fvalN.squaredNorm() / 2;
                gradientN = jacobianN.transpose() * fvalN;

                armijoCondition = errorN <= error + _param.armijoConstant() * stepSize * stepGrad;
                wolfeCondition = gradientN.dot(step) >= _param.wolfeConstant() * stepGrad;

                ++iterations;
            }

            step *= stepSize;
        }
    private:
        const Parameter &_param;
    };

    /// Step refinement method which implements Powell' Dogleg Method.
    struct DoglegMethod { };

    /// Parametrization class for the DoglegMethod refinement method.
    template<typename _Scalar>
    class DoglegMethodParameter
    {
    public:
        using Scalar = _Scalar;

        DoglegMethodParameter() = default;

        DoglegMethodParameter(const Scalar initRadius,
                              const Scalar maxRadius,
                              const Scalar radiusEps,
                              const Scalar acceptFitness,
                              const Index iterations)
            : _initialRadius(initRadius), _maxRadius(maxRadius), _radiusEps(radiusEps),
            _acceptFitness(acceptFitness), _maxIt(iterations)
        { }

        /// Set maximum iterations of the trust region radius search.
        /// Set to 0 or negative for infinite iterations.
        /// @param iterations maximum iterations for radius search
        void setMaximumIterations(const Index iterations)
        {
            _maxIt = iterations;
        }

        /// Returns the maximum iterations of the trust region radius search.
        /// @return maximum iterations for radius search
        Index maximumIterations() const
        {
            return _maxIt;
        }

        /// Set the minimum fitness value at which a model is accepted.
        /// The model fitness is computed as follows:
        ///
        /// fitness = f(xval) - f(xval + step) / m(0) - m(step)
        ///
        /// Where f(x) is the objective error function and m(x) is the
        /// model function describe by the trust region method.
        ///
        /// @param fitness minimum fitness for step acceptance
        void setAcceptanceFitness(const Scalar fitness)
        {
            _acceptFitness = fitness;
        }

        /// Returns the minimum fitness value at which a model is accepted.
        /// @return minimum fitness for step acceptance
        Scalar acceptanceFitness() const
        {
            return _acceptFitness;
        }

        /// Sets the comparison epsilon on how close the step should be
        /// to the trust region radius to trigger an increase of the radius.
        /// Should usually be picked low, e.g. 1e-8.
        /// @param eps comparison epsilon for radius increase
        void setRadiusEpsilon(const Scalar eps)
        {
            _radiusEps = eps;
        }

        /// Returns the comparison epsilon on how close the step should be
        /// to the trust region radius to trigger an increase of the radius.
        /// @return comparison epsilon for radius increase
        Scalar radiusEpsilon() const
        {
            return _radiusEps;
        }

        /// Sets the maximum radius that is used for trust region search.
        /// @param radius maximum trust region radius
        void setMaximumRadius(const Scalar radius)
        {
            _initialRadius = std::min(_initialRadius, radius);
            _maxRadius = radius;
        }

        /// Returns the maximum radius that is used for trust region search.
        /// @return maximum trust region radius
        Scalar maximumRadius() const
        {
            return _maxRadius;
        }

        /// Sets the radius that is initially used by the dogleg method.
        /// The radius must be greater than zero.
        /// @param radius the initial radius that should be used.
        void setInitialRadius(const Scalar radius)
        {
            assert(radius > 0);
            _initialRadius = radius;
        }

        /// Returns the radius that is initially used by the dogleg method.
        /// @return the initial radius
        Scalar initialRadius() const
        {
            return _initialRadius;
        }

    private:
        Scalar _initialRadius = {1};
        Scalar _maxRadius = Scalar{2};
        Scalar _radiusEps = static_cast<Scalar>(1e-6);
        Scalar _acceptFitness = static_cast<Scalar>(0.25);
        Index _maxIt = 0;
    };

    /// Implementation of Powell's Dogleg Method.
    template<typename _Scalar, int _Inputs, int _Outputs>
    class NewtonStepRefiner<_Scalar, _Inputs, _Outputs, DoglegMethod>
    {
    public:
        using Scalar = _Scalar;
        static constexpr int Inputs = _Inputs;
        static constexpr int Outputs = _Outputs;
        using Method = DoglegMethod;
        using Parameter = DoglegMethodParameter<Scalar>;

        static_assert(Eigen::NumTraits<Scalar>::IsInteger == 0, "Step refinement only supports non-integer scalars");

        using InputVector = Eigen::Matrix<Scalar, Inputs, 1>;
        using OutputVector = Eigen::Matrix<Scalar, Outputs, 1>;
        using JacobiMatrix = Eigen::Matrix<Scalar, Outputs, Inputs>;
        using HessianMatrix = Eigen::Matrix<Scalar, Inputs, Inputs>;
        using GradientVector = Eigen::Matrix<Scalar, Inputs, 1>;
        using StepVector = Eigen::Matrix<Scalar, Inputs, 1>;

        NewtonStepRefiner(const Parameter &param)
            : _param(param), _radius(param.initialRadius())
        { }

        template<typename Objective>
        void operator()(const InputVector &xval,
                        const OutputVector &fval,
                        const JacobiMatrix &jacobian,
                        const GradientVector &gradient,
                        const Objective &objective,
                        StepVector &step)
        {
            // approximate hessian
            const HessianMatrix hessian = jacobian.transpose() * jacobian;

            // precompute the full step length
            const StepVector fullStep = step;
            const auto fullStepLenSq = fullStep.squaredNorm();

            // compute the cauchy step
            const auto gradientLenSq = gradient.squaredNorm();
            const Scalar curvature = gradient.dot(hessian * gradient);
            const StepVector cauchyStep = (gradientLenSq / curvature) * gradient;
            const auto cauchyStepLenSq = cauchyStep.squaredNorm();

            // compute step diff
            const StepVector diffStep = fullStep - cauchyStep;
            const auto diffLenSq = diffStep.squaredNorm();
            const Scalar diffFac = cauchyStep.dot(diffStep) / diffLenSq;

            auto modelFitness = _param.acceptanceFitness() - Scalar{1};
            Index iteration = 0;

            // keep computing while the model fitness is bad
            while(modelFitness < _param.acceptanceFitness() &&
                  (_param.maximumIterations() <= Index{0} || iteration < _param.maximumIterations()))
            {
                const auto radiusSq = _radius * _radius;

                // if the full step is within the trust region simply
                // use it, it provides a good minimizer
                if(fullStepLenSq <= radiusSq)
                {
                    step = fullStep;
                }
                else
                {
                    // if the cauchy step lies outside the trust region
                    // go towards it until the trust region boundary
                    if(cauchyStepLenSq >= radiusSq)
                    {
                        step = (_radius / std::sqrt(cauchyStepLenSq)) * cauchyStep;
                    }
                    else
                    {
                        const auto secondTerm = std::sqrt(diffFac * diffFac + (radiusSq + cauchyStepLenSq) / diffLenSq);
                        const auto scale1 = -diffFac - secondTerm;
                        const auto scale2 = -diffFac + secondTerm;

                        step = cauchyStep + std::max(scale1, scale2) * (fullStep - cauchyStep);
                    }
                }

                // compute the model fitness to determine the update scheme for
                // the trust region radius
                modelFitness = calulateModelFitness(xval, fval, gradient, hessian, step, objective);

                const auto stepLen = step.norm();

                // if the model fitness is really bad reduce the radius!
                if(modelFitness < static_cast<Scalar>(0.25))
                {
                    _radius = static_cast<Scalar>(0.25) * stepLen;
                }
                // if the model fitness is very good then increase it
                else if(modelFitness > static_cast<Scalar>(0.75) && std::abs(stepLen - _radius) < _param.radiusEpsilon())
                {
                    // use the double radius
                    _radius = 2 * _radius;
                    // maintain radius border if configured
                    if(_param.maximumRadius() > 0 && _radius > _param.maximumRadius())
                        _radius = _param.maximumRadius();
                }

                ++iteration;
            }
        }

    private:
        const Parameter &_param;
        Scalar _radius = Scalar{1};

        template<typename Objective>
        Scalar calulateModelFitness(const InputVector &xval,
                                    const OutputVector &fval,
                                    const GradientVector &gradient,
                                    const HessianMatrix &hessian,
                                    const StepVector &step,
                                    const Objective &objective) const
        {
            const Scalar error = fval.squaredNorm() / Scalar{2};

            // evaluate the error function at the new position
            InputVector xvalNext = xval + step;
            OutputVector fvalNext;
            JacobiMatrix jacobianNext;
            objective(xvalNext, fvalNext, jacobianNext);
            // compute the actual new error
            const Scalar nextError = fvalNext.squaredNorm() / Scalar{2};
            // compute the new error by the model
            const Scalar modelError = error + gradient.dot(step) + step.dot(hessian * step) / Scalar{2};

            return (error - nextError) / (error - modelError);
        }
    };

    /// Parameterization class for the GradientDescent step calculation method.
    struct GradientDescentParameter {};

    /// Gradient descent step calculation method.
    struct GradientDescentMethod
    {
        using Parameter = GradientDescentParameter;

        GradientDescentMethod(const Parameter &)
        { }

        /// Computes the newton step as the gradient of the objective function.
        /// @param gradient gradient of the objective function
        /// @param step computed newton step
        /// @return true on success, otherwise false
        template<typename I, typename O, typename J, typename G, typename Objective, typename S>
        bool operator()(const Eigen::MatrixBase<I>&,
                        const Eigen::MatrixBase<O> &,
                        const Eigen::MatrixBase<J> &,
                        const Eigen::MatrixBase<G> &gradient,
                        const Objective &,
                        Eigen::MatrixBase<S>& step) const
        {
            step = gradient;
            return true;
        }
    };

    /// Parameterization class for the GaussNewton step calculation method.
    struct GaussNewtonParameter {};

    /// Gradient descent step calculation method.
    template<typename _Solver=DenseSVDSolver>
    struct GaussNewtonMethod
    {
        using Parameter = GaussNewtonParameter;
        using Solver = _Solver;

        GaussNewtonMethod(const Parameter &)
        { }

        /// Computes the newton step from a linearized approximation of the objective function.
        /// @param gradient gradient of the objective function
        /// @param step computed newton step
        /// @return true on success, otherwise false
        template<typename I, typename O, typename J, typename G, typename Objective, typename S>
        bool operator()(const Eigen::MatrixBase<I> &,
                        const Eigen::MatrixBase<O> &,
                        const Eigen::MatrixBase<J> &jacobian,
                        const Eigen::MatrixBase<G> &gradient,
                        const Objective &,
                        Eigen::MatrixBase<S>& step) const
        {
            Solver solver;
            const auto A = jacobian.transpose() * jacobian;
            using Matrix = typename decltype(A)::PlainMatrix;
            return solver(Matrix(A), gradient, step);
        }
    };

    /// Parameterization class for the LevenbergMarquardt step calculation method.
    template<typename _Scalar>
    class LevenbergMarquardtParameter
    {
    public:
        using Scalar = _Scalar;

        LevenbergMarquardtParameter() = default;

        LevenbergMarquardtParameter(const Scalar initialLambda,
                                    const Scalar increase,
                                    const Scalar decrease,
                                    const Index iterations)
            : _initialLambda(initialLambda), _increase(increase), _decrease(decrease), _maxIt(iterations)
        {
            assert(initialLambda > Scalar{0});
            assert(decrease < Scalar{1});
            assert(decrease > Scalar{0});
            assert(increase > Scalar{1});
        }

        /// Sets the initial gradient descent factor of levenberg marquardt.
        /// @param lambda gradient descent factor
        void setInitialLambda(const Scalar lambda)
        {
            assert(lambda > Scalar{0});
            _initialLambda = lambda;
        }

        /// Returns the initial gradient descent factor of levenberg marquardt.
        /// @return lambda gradient descent factor
        Scalar initialLambda() const
        {
            return _initialLambda;
        }

        /// Sets the maximum iterations of the levenberg marquardt optimization.
        /// Set to 0 or negative for infinite iterations.
        /// @param iterations maximum iterations for lambda search
        void setMaximumIterations(const Index iterations)
        {
            _maxIt = iterations;
        }

        /// Returns the maximum iterations of the levenberg marquardt optimization.
        /// @return maximum iterations for lambda search
        Index maximumIterations() const
        {
            return _maxIt;
        }

        /// Sets the increase factor for the lambda damping.
        /// The value has to be greater than 1.
        /// @param increase factor for increasing lambda
        void setIncrease(const Scalar increase)
        {
            assert(increase > Scalar{1});
            _increase = increase;
        }

        /// Returns the increase factor for the lambda damping.
        /// @return increase factor for increasing lambda
        Scalar increase() const
        {
            return _increase;
        }

        /// Sets the decrease factor for the lambda damping.
        /// The value has to be in (0, 1).
        /// @param decrease factor for decreasing lambda
        void setDecrease(const Scalar decrease)
        {
            assert(decrease < Scalar{1});
            assert(decrease > Scalar{0});
            _decrease = decrease;
        }

        /// Returns the decrease factor for the lambda damping.
        /// @return factor for increasing lambda
        Scalar decrease() const
        {
            return _decrease;
        }
    private:
        Scalar _initialLambda = Scalar{1};
        Scalar _increase = static_cast<Scalar>(2);
        Scalar _decrease = static_cast<Scalar>(0.5);
        Index _maxIt = 0;
    };

    /// Step refinement method which implements the Levenberg Marquardt method.
    template<typename _Scalar, typename _Solver=DenseSVDSolver>
    class LevenbergMarquardtMethod
    {
    public:
        using Scalar = _Scalar;
        using Solver = _Solver;
        using Parameter = LevenbergMarquardtParameter<Scalar>;

        LevenbergMarquardtMethod(const Parameter &param)
            : _param(param), _lambda(param.initialLambda())
        { }

        /// Computes the newton step from a linearized approximation of the objective function.
        /// @param gradient gradient of the objective function
        /// @param step computed newton step
        /// @return true on success, otherwise false
        template<typename I, typename O, typename J, typename G, typename Objective, typename S>
        bool operator()(const Eigen::MatrixBase<I> &xval,
                        const Eigen::MatrixBase<O> &fval,
                        const Eigen::MatrixBase<J> &jacobian,
                        const Eigen::MatrixBase<G> &gradient,
                        const Objective &objective,
                        Eigen::MatrixBase<S>& step)
        {
            using InputVector = typename Eigen::MatrixBase<I>::PlainMatrix;
            using OutputVector = typename Eigen::MatrixBase<O>::PlainMatrix;
            using JacobiMatrix = typename Eigen::MatrixBase<J>::PlainMatrix;
            using SystemMatrix = typename decltype(jacobian.transpose() * jacobian)::PlainMatrix;

            const auto error = fval.squaredNorm() / 2;
            const SystemMatrix jacobianSq = jacobian.transpose() * jacobian;

            Scalar errorN = error + 1;
            SystemMatrix A;
            InputVector xvalN;
            OutputVector fvalN;
            JacobiMatrix jacobianN;
            Solver solver;

            Index iterations = 0;
            while((_param.maximumIterations() <= 0 || iterations < _param.maximumIterations()) &&
                errorN > error)
            {
                A = jacobianSq;
                // add identity matrix
                for(Index i = 0; i < A.rows(); ++i)
                    A(i, i) += _lambda;

                const auto ret = solver(A, gradient, step);
                if(!ret)
                    return false;

                xvalN = xval - step;
                objective(xvalN, fvalN, jacobianN);
                errorN = fvalN.squaredNorm() / 2;

                if(errorN > error)
                    _lambda *= _param.increase();
                else
                    _lambda *= _param.decrease();

                ++iterations;
            }

            return true;
        }

    private:
        Parameter _param = {};
        Scalar _lambda = Scalar{1};
    };

    namespace internal
    {
        template<bool ComputesJacobian>
        struct ObjectiveEvaluator
        { };

        template<>
        struct ObjectiveEvaluator<true>
        {
            template<typename InputVector, typename Objective, typename FiniteDifferences, typename Params, typename OutputVector, typename JacobiMatrix>
            void operator()(const InputVector &xval,
                            const Objective &objective,
                            const FiniteDifferences&,
                            const Params &,
                            OutputVector &fval,
                            JacobiMatrix &jacobian) const
            {
                objective(xval, fval, jacobian);
            }
        };

        template<>
        struct ObjectiveEvaluator<false>
        {
            template<typename InputVector, typename Objective, typename FiniteDifferences, typename Params, typename OutputVector, typename JacobiMatrix>
            void operator()(const InputVector &xval,
                            const Objective &objective,
                            const FiniteDifferences &finiteDifferences,
                            const Params &param,
                            OutputVector &fval,
                            JacobiMatrix &jacobian) const
            {
                objective(xval, fval);
                finiteDifferences(xval, fval, objective, param, jacobian);
            }
        };

        template <typename T>
        bool almostZero(T x)
        {
            return std::abs(x) < std::numeric_limits<T>::epsilon();
        }

        template<typename Derived>
        std::string makePrettyVector(const Eigen::MatrixBase<Derived> &vec)
        {
            assert(vec.cols() == 1);

            std::stringstream ss1;
            ss1 << std::fixed << std::showpoint << std::setprecision(6);
            std::stringstream ss2;
            ss2 << '[';
            for(Index i = 0; i < vec.rows(); ++i)
            {
                ss1 << vec(i, 0);
                ss2 << std::setfill(' ') << std::setw(10) << ss1.str();
                if(i != vec.rows() - 1)
                    ss2 << ' ';
                ss1.str("");
            }
            ss2 << ']';

            return ss2.str();
        }
    }

    /// Core class for least squares algorithms.
    /// It implements the whole optimization strategy except the step
    /// calculation and its refinement.
    template<typename _Scalar,
             int _Inputs,
             int _Outputs,
             typename _Objective,
             typename _StepMethod,
             typename _RefineMethod,
             typename _FiniteDifferencesMethod>
    class LeastSquaresAlgorithm
    {
    public:
        using Scalar = _Scalar;
        constexpr static auto Inputs = _Inputs;
        constexpr static auto Outputs = _Outputs;
        using Objective = _Objective;

        using StepMethod = _StepMethod;
        using StepMethodParameter = typename StepMethod::Parameter;

        using StepRefiner = NewtonStepRefiner<Scalar, Inputs, Outputs, _RefineMethod>;
        using StepRefinerParameter = typename StepRefiner::Parameter;

        using FiniteDifferencesMethod = _FiniteDifferencesMethod;

        using InputVector = Eigen::Matrix<Scalar, Inputs, 1>;
        using OutputVector = Eigen::Matrix<Scalar, Outputs, 1>;
        using JacobiMatrix = Eigen::Matrix<Scalar, Outputs, Inputs>;
        using HessianMatrix = Eigen::Matrix<Scalar, Inputs, Inputs>;
        using GradientVector = Eigen::Matrix<Scalar, Inputs, 1>;
        using StepVector = Eigen::Matrix<Scalar, Inputs, 1>;



        using Callback = std::function<bool(const Index,
                                            const InputVector&,
                                            const OutputVector&,
                                            const JacobiMatrix&,
                                            const GradientVector&,
                                            const StepVector&)>;

        /// Result data returned after optimization.
        struct Result
        {
            InputVector xval = {};
            OutputVector fval = {};
            Scalar error = Scalar{-1};
            Index iterations = -1;
            bool converged = false;
            bool succeeded = false;
        };

        LeastSquaresAlgorithm() = default;

        /// Set the number of threads used to compute gradients.
        /// This only works if OpenMP is enabled.
        /// Set to 0 to allow automatic detection of thread number.
        /// @param threads number of threads to be used
        void setThreads(const Index threads)
        {
            _finiteDifferencesParam.setThreads(threads);
        }

        /// Set the difference for gradient estimation with finite differences.
        /// @param eps numerical epsilon
        void setNumericalEpsilon(const Scalar eps)
        {
            _finiteDifferencesParam.setNumericalEpsilon(eps);
        }

        /// Sets the parameters for the newton step calculation method, e.g. GaussNewton, LevenbergMarquardt, etc.
        /// @param param parameters that should be set.
        void setMethodParameters(const StepMethodParameter &param)
        {
            _methodParam = param;
        }

        /// Returns the parameters for the newton step calculation method.
        /// @return step calculation parameters
        const StepMethodParameter &methodParameters() const
        {
            return _methodParam;
        }

        /// Sets the parameters for the newton step refinement method, e.g. ArmijoBacktracking, DoglegMethod, etc.
        /// @param param parameters that should be set.
        void setRefinementParameters(const StepRefinerParameter &param)
        {
            _refinerParam = param;
        }

        /// Returns the parameters for the newton step refinement method.
        /// @return step calculation parameters
        const StepRefinerParameter &refinementParameters() const
        {
            return _refinerParam;
        }

        /// Sets the instance values of the custom objective function.
        /// Should be used if the objective function requires custom data parameters.
        /// @param objective instance that should be copied
        void setObjective(const Objective &objective)
        {
            _objective = objective;
        }

        /// Sets the instance functor for iteration callbacks.
        /// @param refiner instance that should be copied
        template<typename T>
        void setCallback(const T &callback)
        {
            _callback = callback;
        }

        /// Sets the maximum number of iterations.
        /// Set to 0 or negative for infinite iterations.
        /// @param iterations maximum number of iterations
        void setMaximumIterations(const Index iterations)
        {
            _maxIt = iterations;
        }

        /// Set the minimum step length between two iterations.
        /// If the step length falls below this value, the optimizer stops.
        /// @param steplen minimum step length
        void setMinimumStepLength(const Scalar steplen)
        {
            _minStepLen = steplen;
        }

        /// Set the minimum gradient length.
        /// If the gradient length falls below this value, the optimizer stops.
        /// @param gradlen minimum gradient length
        void setMinimumGradientLength(const Scalar gradlen)
        {
            _minGradLen = gradlen;
        }

        /// Set the minimum squared error.
        /// If the error falls below this value, the optimizer stops.
        /// @param error minimum error
        void setMinimumError(const Scalar error)
        {
            _minError = error;
        }

        /// Set the level of verbosity to print status information after each
        /// iteration.
        /// Set to 0 to deacticate any output.
        /// @param verbosity level of verbosity
        void setVerbosity(const Index verbosity)
        {
            _verbosity = verbosity;
        }

        /// Sets the output stream that is used to print the optimization progress.
        /// @param output the output stream
        void setOutputStream(std::ostream &output)
        {
            _output = &output;
        }

        /// Minimizes the configured objective starting at the given initial guess.
        Result minimize(const InputVector &initialGuess)
        {
            InputVector xval = initialGuess;
            OutputVector fval;
            JacobiMatrix jacobian;
            GradientVector gradient;
            StepVector step = StepVector::Zero(xval.size());

            auto gradLen = _minGradLen + 1;
            auto error = _minError + 1;
            auto stepLen = _minStepLen + 1;
            bool callbackResult = true;
            bool succeeded = true;

            // create objective lambda function which performs automatically numerical estimation
            // of the gradient if necessary.
            const auto objective = [this](const InputVector &xval, OutputVector &fval, JacobiMatrix &jacobian)
            {
                const auto evaluator = internal::ObjectiveEvaluator<Objective::ComputesJacobian>();
                evaluator(xval, this->_objective, FiniteDifferencesMethod(), this->_finiteDifferencesParam, fval, jacobian);
            };

            auto stepMethod = StepMethod(_methodParam);
            auto stepRefiner = StepRefiner(_refinerParam);

            Index iterations = 0;
            while((_maxIt <= 0 || iterations < _maxIt) &&
                   gradLen >= _minGradLen &&
                   stepLen >= _minStepLen &&
                   error >= _minError &&
                   callbackResult)
            {
                xval -= step;
                objective(xval, fval, jacobian);

                error = fval.squaredNorm() / 2;
                gradient = jacobian.transpose() * fval;
                gradLen = gradient.norm();

                // compute the full newton step according to the current method
                if(!stepMethod(xval, fval, jacobian, gradient, objective, step))
                {
                    succeeded = false;
                    break;
                }

                // refine the step according to the current refiner
                stepRefiner(xval, fval, jacobian, gradient, objective, step);
                stepLen = step.norm();

                // evaluate callback if available
                if(_callback)
                {
                    callbackResult = _callback(iterations + 1, xval, fval, jacobian, gradient, step);
                }

                if(_verbosity > 0)
                {
                    using internal::makePrettyVector;

                    std::stringstream ss;
                    ss << "it=" << std::setfill('0')
                        << std::setw(4) << iterations
                        << std::fixed << std::showpoint << std::setprecision(6)
                        << "    steplen=" << stepLen
                        << "    gradlen=" << gradLen;

                    if(_verbosity > 1)
                        ss << "    callback=" << (callbackResult ? "true" : "false");

                    ss << "    error=" << error;

                    if(_verbosity > 2)
                        ss << "    xval=" << makePrettyVector(xval);
                    if(_verbosity > 3)
                        ss << "    step=" << makePrettyVector(step);
                    if(_verbosity > 4)
                        ss << "    fval=" << makePrettyVector(fval);
                    (*_output) << ss.str() << std::endl;
                }

                ++iterations;
            }

            Result result;
            result.xval = xval;
            result.fval = fval;
            result.error = error;
            result.iterations = iterations;
            result.succeeded = succeeded;
            result.converged = stepLen < _minStepLen ||
                gradLen < _minGradLen ||
                error < _minError;

            return result;
        }

    private:
        Objective _objective = {};
        Callback _callback = {};

        StepMethodParameter _methodParam = {};
        StepRefinerParameter _refinerParam = {};
        FiniteDifferencesParameter<Scalar> _finiteDifferencesParam = {};

        Index _maxIt = 0;
        Scalar _minStepLen = static_cast<Scalar>(1e-9);
        Scalar _minGradLen = static_cast<Scalar>(1e-9);
        Scalar _minError = Scalar{0};
        Index _verbosity = 0;
        std::ostream *_output = &std::cout;
    };

    /// General Gauss Newton algorithm.
    template<typename Scalar,
             int Inputs,
             int Outputs,
             typename Objective,
             typename RefineMethod=ConstantStepFactor,
             typename Solver=DenseSVDSolver,
             typename FiniteDifferencesMethod=CentralDifferences>
    using GaussNewton = LeastSquaresAlgorithm<Scalar,
                                              Inputs,
                                              Outputs,
                                              Objective,
                                              GaussNewtonMethod<Solver>,
                                              RefineMethod,
                                              FiniteDifferencesMethod>;

    /// Gauss Newton algorithm with dynamic problem size.
    template<typename Scalar,
             typename Objective,
             typename RefineMethod=ConstantStepFactor,
             typename Solver=DenseSVDSolver,
             typename FiniteDifferencesMethod=CentralDifferences>
    using GaussNewtonX = GaussNewton<Scalar,
                                     Eigen::Dynamic,
                                     Eigen::Dynamic,
                                     Objective,
                                     RefineMethod,
                                     Solver,
                                     FiniteDifferencesMethod>;

    /// General Gradient Descent algorithm.
    template<typename Scalar,
             int Inputs,
             int Outputs,
             typename Objective,
             typename RefineMethod=ConstantStepFactor,
             typename FiniteDifferencesMethod=CentralDifferences>
    using GradientDescent = LeastSquaresAlgorithm<Scalar,
                                                  Inputs,
                                                  Outputs,
                                                  Objective,
                                                  GradientDescentMethod,
                                                  RefineMethod,
                                                  FiniteDifferencesMethod>;

    /// Gradient Descent algorithm with dynamic problem size.
    template<typename Scalar,
             typename Objective,
             typename RefineMethod=ConstantStepFactor,
             typename FiniteDifferencesMethod=CentralDifferences>
    using GradientDescentX = GradientDescent<Scalar,
                                             Eigen::Dynamic,
                                             Eigen::Dynamic,
                                             Objective,
                                             RefineMethod,
                                             FiniteDifferencesMethod>;

    /// General Levenberg Marquardt algorithm.
    template<typename Scalar,
             int Inputs,
             int Outputs,
             typename Objective,
             typename Solver=DenseSVDSolver,
             typename FiniteDifferencesMethod=CentralDifferences>
    using LevenbergMarquardt = LeastSquaresAlgorithm<Scalar,
                                              Inputs,
                                              Outputs,
                                              Objective,
                                              LevenbergMarquardtMethod<Scalar, Solver>,
                                              ConstantStepFactor,
                                              FiniteDifferencesMethod>;

    template<typename Scalar,
             typename Objective,
             typename Solver=DenseSVDSolver,
             typename FiniteDifferencesMethod=CentralDifferences>
    using LevenbergMarquardtX = LevenbergMarquardt<Scalar,
                                     Eigen::Dynamic,
                                     Eigen::Dynamic,
                                     Objective,
                                     Solver,
                                     FiniteDifferencesMethod>;

    namespace parameter
    {
        /// Encodes a 3x3 rotation matrix into a 3-vector in angle axis representation using the rodrigues' formula.
        /// This parametrization is minimal, free of singularities and gimbal lock.
        /// @note https://en.wikipedia.org/wiki/Axis%E2%80%93angle_representation
        template<typename Derived>
        Eigen::Matrix<typename Eigen::MatrixBase<Derived>::Scalar, 3, 1> encodeRotation(const Eigen::MatrixBase<Derived> &rotation)
        {
            using Scalar = typename Eigen::MatrixBase<Derived>::Scalar;
            using Vector3 = Eigen::Matrix<Scalar, 3, 1>;
            assert(rotation.rows() == 3);
            assert(rotation.cols() == 3);

            const auto theta = std::acos((rotation.trace() - Scalar{1}) / Scalar{2});

            if(internal::almostZero(theta))
            {
                return Vector3::Zero();
            }
            else
            {
                const auto fac = theta / (Scalar{2} * std::sin(theta));

                return fac * Vector3(rotation(2, 1) - rotation(1, 2),
                                     rotation(0, 2) - rotation(2, 0),
                                     rotation(1, 0) - rotation(0, 1));
            }

        }

        /// Decodes a 3-vector in angle axis representation into a 3x3 rotation matrix using the rodrigues' formula.
        /// @note https://en.wikipedia.org/wiki/Axis%E2%80%93angle_representation
        template<typename Derived>
        Eigen::Matrix<typename Eigen::MatrixBase<Derived>::Scalar, 3, 3> decodeRotation(const Eigen::MatrixBase<Derived> &rotation)
        {
            using Scalar = typename Eigen::MatrixBase<Derived>::Scalar;
            using Matrix3 = Eigen::Matrix<Scalar, 3, 3>;
            using Vector3 = Eigen::Matrix<Scalar, 3, 1>;

            assert(rotation.rows() == 3);
            assert(rotation.cols() == 1);

            const auto theta = rotation.norm();
            if(internal::almostZero(theta))
            {
                return Matrix3::Identity();
            }
            else
            {
                const Vector3 w = rotation / theta;

                const auto s = std::sin(theta);
                const auto c = std::cos(theta);

                Matrix3 K;
                K << 0, -w.z(), w.y(),
                    w.z(), 0,  -w.x(),
                    -w.y(), w.x(), 0;

                // Matrix3 result;
                // result << c + wx * wx * (1 - c),      wx * wy * (1 - c) - wz * s, wy * s + wx * wz * (1 - c),
                //           wz * s + wx * wy * (1 - c), c + wy * wy * (1 - c), -wx * s + wy * wz * (1 - c),
                //           -wy * s + wx * wz * (1 - c), wx * s + wy * wz * (1 - c), c + wz * wz * (1 - c);

                return Matrix3::Identity() + s * K + (1 - c) * (w * w.transpose() - Matrix3::Identity());
            }
        }
    }
}

#endif
