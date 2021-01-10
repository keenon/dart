/*
 * Copyright (c) 2011-2019, The DART development contributors
 * All rights reserved.
 *
 * The list of contributors can be found at:
 *   https://github.com/dartsim/dart/blob/master/LICENSE
 *
 * This file is provided under the following "BSD-style" License:
 *   Redistribution and use in source and binary forms, with or
 *   without modification, are permitted provided that the following
 *   conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 *   CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 *   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 *   USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 *   AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *   POSSIBILITY OF SUCH DAMAGE.
 */

#include "dart/constraint/BoxedLcpConstraintSolver.hpp"

#include <cassert>
#ifndef NDEBUG
#include <iomanip>
#include <iostream>
#endif

#include "dart/common/Console.hpp"
#include "dart/constraint/ConstraintBase.hpp"
#include "dart/constraint/DantzigBoxedLcpSolver.hpp"
#include "dart/constraint/LCPUtils.hpp"
#include "dart/constraint/PgsBoxedLcpSolver.hpp"
#include "dart/external/odelcpsolver/lcp.h"
#include "dart/lcpsolver/Lemke.hpp"
#include "dart/neural/ConstrainedGroupGradientMatrices.hpp"

namespace dart {
namespace constraint {

//==============================================================================
BoxedLcpConstraintSolver::BoxedLcpConstraintSolver(
    double timeStep,
    BoxedLcpSolverPtr boxedLcpSolver,
    BoxedLcpSolverPtr secondaryBoxedLcpSolver)
  : BoxedLcpConstraintSolver(
      std::move(boxedLcpSolver), std::move(secondaryBoxedLcpSolver))
{
  setTimeStep(timeStep);
}

//==============================================================================
BoxedLcpConstraintSolver::BoxedLcpConstraintSolver()
  : BoxedLcpConstraintSolver(std::make_shared<DantzigBoxedLcpSolver>())
{
  // Do nothing
}

//==============================================================================
BoxedLcpConstraintSolver::BoxedLcpConstraintSolver(
    BoxedLcpSolverPtr boxedLcpSolver)
  : BoxedLcpConstraintSolver(
      std::move(boxedLcpSolver), std::make_shared<PgsBoxedLcpSolver>())
{
  // Do nothing
}

//==============================================================================
BoxedLcpConstraintSolver::BoxedLcpConstraintSolver(
    BoxedLcpSolverPtr boxedLcpSolver, BoxedLcpSolverPtr secondaryBoxedLcpSolver)
  : ConstraintSolver()
{
  if (boxedLcpSolver)
  {
    setBoxedLcpSolver(std::move(boxedLcpSolver));
  }
  else
  {
    dtwarn << "[BoxedLcpConstraintSolver] Attempting to construct with nullptr "
           << "LCP solver, which is not allowed. Using Dantzig solver "
           << "instead.\n";
    setBoxedLcpSolver(std::make_shared<DantzigBoxedLcpSolver>());
  }

  setSecondaryBoxedLcpSolver(std::move(secondaryBoxedLcpSolver));
}

//==============================================================================
void BoxedLcpConstraintSolver::setBoxedLcpSolver(BoxedLcpSolverPtr lcpSolver)
{
  if (!lcpSolver)
  {
    dtwarn << "[BoxedLcpConstraintSolver::setBoxedLcpSolver] "
           << "nullptr for boxed LCP solver is not allowed.";
    return;
  }

  if (lcpSolver == mSecondaryBoxedLcpSolver)
  {
    dtwarn << "[BoxedLcpConstraintSolver::setBoxedLcpSolver] Attempting to set "
           << "a primary LCP solver that is the same with the secondary LCP "
           << "solver, which is discouraged. Ignoring this request.\n";
  }

  mBoxedLcpSolver = std::move(lcpSolver);
}

//==============================================================================
/// This is pretty much only useful for Finite Difference testing with very
/// complex collisions. This replaces the standard LCP solver with a PGS
/// algorithm dialed up to super-duper-accurate max settings.
void BoxedLcpConstraintSolver::makeHyperAccurateAndVerySlow()
{
  std::shared_ptr<PgsBoxedLcpSolver> accurateAndSlowSolver
      = std::make_shared<PgsBoxedLcpSolver>();
  /*
  accurateAndSlowSolver->setOption(
      PgsBoxedLcpSolver::Option(50000, 1e-15, 1e-12, 1e-10, false));
  */
  accurateAndSlowSolver->setOption(
      PgsBoxedLcpSolver::Option(1000, 1e-10, 1e-8, 1e-8, false));
  setBoxedLcpSolver(accurateAndSlowSolver);
}

//==============================================================================
ConstBoxedLcpSolverPtr BoxedLcpConstraintSolver::getBoxedLcpSolver() const
{
  return mBoxedLcpSolver;
}

//==============================================================================
void BoxedLcpConstraintSolver::setSecondaryBoxedLcpSolver(
    BoxedLcpSolverPtr lcpSolver)
{
  if (lcpSolver == mBoxedLcpSolver)
  {
    dtwarn << "[BoxedLcpConstraintSolver::setBoxedLcpSolver] Attempting to set "
           << "the secondary LCP solver that is identical to the primary LCP "
           << "solver, which is redundant. Please use different solvers or set "
           << "the secondary LCP solver to nullptr.\n";
  }

  mSecondaryBoxedLcpSolver = std::move(lcpSolver);
}

//==============================================================================
ConstBoxedLcpSolverPtr BoxedLcpConstraintSolver::getSecondaryBoxedLcpSolver()
    const
{
  return mSecondaryBoxedLcpSolver;
}

//==============================================================================
void BoxedLcpConstraintSolver::solveConstrainedGroup(
    ConstrainedGroup& group, simulation::World* world)
{
  // Build LCP terms by aggregating them from constraints
  const std::size_t numConstraints = group.getNumConstraints();
  const std::size_t n = group.getTotalDimension();

  // If there is no constraint, then just return.
  if (0u == n)
    return;

  const int nSkip = dPAD(n); // nSkip = n + (n % 4);
#ifdef NDEBUG                // release
  mA.resize(n, nSkip);
#else // debug
  mA.setZero(n, nSkip); // rows = n, cols = n + (n % 4)
#endif
  bool mXResized = mX.size() != n;
  if (mXResized)
  {
    mX.resize(n);
    mX.setZero();
  }
  mB.resize(n);
  mW.setZero(n); // set w to 0
  mLo.resize(n);
  mHi.resize(n);
  mFIndex.setConstant(n, -1); // set findex to -1

  // Compute offset indices
  mOffset.resize(n);
  mOffset[0] = 0;
  for (std::size_t i = 1; i < numConstraints; ++i)
  {
    const ConstraintBasePtr& constraint = group.getConstraint(i - 1);
    assert(constraint->getDimension() > 0);
    mOffset[i] = mOffset[i - 1] + constraint->getDimension();
  }

  // For each constraint
  ConstraintInfo constInfo;
  constInfo.invTimeStep = 1.0 / mTimeStep;
  for (std::size_t i = 0; i < numConstraints; ++i)
  {
    const ConstraintBasePtr& constraint = group.getConstraint(i);

    constInfo.x = mX.data() + mOffset[i];
    constInfo.lo = mLo.data() + mOffset[i];
    constInfo.hi = mHi.data() + mOffset[i];
    constInfo.b = mB.data() + mOffset[i];
    constInfo.findex = mFIndex.data() + mOffset[i];
    constInfo.w = mW.data() + mOffset[i];

    // Fill vectors: lo, hi, b, w
    constraint->getInformation(&constInfo);

    // Register this constraint with our gradient matrices. It's important that
    // this be called _after_ the getInformation() call, because it relies on
    // state being filled from that call.
    if (group.getGradientConstraintMatrices())
    {
      group.getGradientConstraintMatrices()->registerConstraint(constraint);
    }

    // Fill a matrix by impulse tests: A
    constraint->excite();

    double* impulses = new double[constraint->getDimension()];
    for (std::size_t j = 0; j < constraint->getDimension(); ++j)
    {
      // Adjust findex for global index
      if (mFIndex[mOffset[i] + j] >= 0)
        mFIndex[mOffset[i] + j] += mOffset[i];

      // Apply impulse for mipulse test
      constraint->applyUnitImpulse(j);

      // Fill upper triangle blocks of A matrix

      // mA is row-major order, n rows by nSkip cols
      // nSkip * (mOffset[i] + j) takes us to the (mOffset[i] + j)'th row
      // mOffset[i] into that row
      //
      // -------------------------------
      //                                |
      //                   nSkip * (mOffset[i] + j)
      //                                |
      //                                v
      // ------- mOffset[i] ----------> xxxxxxxxxx
      //
      // This whole loop fills the entire row (mOffset[i] + j) of A with
      // the effect that the unit impluse on constraint for j for this
      // constraint has on the relative velocities for each constraint
      // force direction.
      //
      // As an efficiency tweak, since you know that A is symmetric, only
      // bother to actually compute half of the velocity changes (upper
      // triangle, arbitrarily) and then just copy that into the other half.

      // Create a 3x3 square from A(mOffset[i], mOffset[i]) iterating over j
      // This iteration fill in row j
      int index = nSkip * (mOffset[i] + j) + mOffset[i];
      constraint->getVelocityChange(
          mA.data() + index, mConstraintForceMixingEnabled);

      for (std::size_t k = i + 1; k < numConstraints; ++k)
      {
        // Create a 3x3 square from A(mOffset[i], mOffset[k]), iterating over j
        // This iteration fill in row j
        // Probably mostly 0s
        index = nSkip * (mOffset[i] + j) + mOffset[k];
        group.getConstraint(k)->getVelocityChange(mA.data() + index, false);
      }

      // Filling symmetric part of A matrix
      for (std::size_t k = 0; k < i; ++k)
      {
        const int indexI = mOffset[i] + j;
        for (std::size_t l = 0; l < group.getConstraint(k)->getDimension(); ++l)
        {
          const int indexJ = mOffset[k] + l;
          // We've already calculate the velocity of
          // mA(column for this constraint, previous constraint row) =
          //     mA(previous constraint row, column for this constraint)
          mA(indexI, indexJ) = mA(indexJ, indexI);
        }
      }

      if (group.getGradientConstraintMatrices())
      {
        group.getGradientConstraintMatrices()->measureConstraintImpulse(
            constraint, j);
      }
    }
    delete[] impulses;

    assert(isSymmetric(
        n, mA.data(), mOffset[i], mOffset[i] + constraint->getDimension() - 1));

    constraint->unexcite();
  }

  assert(isSymmetric(n, mA.data()));

  // Print LCP formulation
  /*
  dtdbg << "Before solve:" << std::endl;
  print(
      n,
      mA.data(),
      mX.data(),
      mLo.data(),
      mHi.data(),
      mB.data(),
      mW.data(),
      mFIndex.data());
  std::cout << std::endl;
  */

  // Solve LCP using the primary solver and fallback to secondary solver when
  // the parimary solver failed.
  if (mSecondaryBoxedLcpSolver)
  {
    // Make backups for the secondary LCP solver because the primary solver
    // modifies the original terms.
    mABackup = mA;
    mXBackup = mX;
    mBBackup = mB;
    mLoBackup = mLo;
    mHiBackup = mHi;
    mFIndexBackup = mFIndex;
  }
  // Always make backups of these variables, regardless of whether we're using
  // a secondary solver, because we need them for gradients
  Eigen::VectorXd loGradientBackup = mLo;
  Eigen::VectorXd hiGradientBackup = mHi;
  Eigen::VectorXi fIndexGradientBackup = mFIndex;
  Eigen::VectorXd bGradientBackup = mB;
  Eigen::VectorXd aColNormGradientBackup = Eigen::VectorXd(n);
  for (std::size_t i = 0; i < n; i++)
  {
    aColNormGradientBackup(i) = mA.col(i).squaredNorm();
  }
  // mA can actually be non-square, for efficiency reasons, so we make sure we
  // keep just the square block.
  Eigen::MatrixXd aGradientBackup = mA.block(0, 0, n, n);

  bool success = false;
  bool shortCircuitLCP = false;

  // Pre-solve, if we're using gradients. We're going to assume that the
  // initialization mX is from last time step, and then guess that nothing has
  // changed categories. If that's true, then we can get a solution in a single
  // matrix inversion.
  //
  // This gives us two advantages:
  //
  // 1) It's less computation in order to get a very accurate result.
  //
  // 2) It keeps classes stable and nicely differentiable, by getting LCP
  // solutions near previous solutions.

  if (group.getGradientConstraintMatrices() && !mXResized)
  {
    std::shared_ptr<neural::ConstrainedGroupGradientMatrices> grads
        = group.getGradientConstraintMatrices();
    grads->registerLCPResults(
        mX, mHi, mLo, mFIndex, mB, aColNormGradientBackup, aGradientBackup);
    grads->constructMatrices(world);
    success = grads->opportunisticallyStandardizeResults(world, mX);
    // If this worked, we don't need to reconstruct our constraint matrices,
    // since the ones we just made already work by construction
    shortCircuitLCP = success;
  }

  // If we were unable to solve the problem by approximation from the previous
  // solution, then re-solve it fully using an LCP
  if (!success)
  {
    const bool earlyTermination = (mSecondaryBoxedLcpSolver != nullptr);
    assert(mBoxedLcpSolver);
    success = mBoxedLcpSolver->solve(
        n,
        mA.data(),
        mX.data(),
        mB.data(),
        0,
        mLo.data(),
        mHi.data(),
        mFIndex.data(),
        earlyTermination);
  }

  // Sanity check. LCP solvers should not report success with nan values, but
  // it could happen. So we set the sucees to false for nan values.
  if (success && mX.hasNaN())
    success = false;

  if (!success && mSecondaryBoxedLcpSolver)
  {
    mSecondaryBoxedLcpSolver->solve(
        n,
        mABackup.data(),
        mXBackup.data(),
        mBBackup.data(),
        0,
        mLoBackup.data(),
        mHiBackup.data(),
        mFIndexBackup.data(),
        false);
    mX = mXBackup;
  }

  if (mX.hasNaN())
  {
    dterr << "[BoxedLcpConstraintSolver] The solution of LCP includes NAN "
          << "values: " << mX.transpose() << ". We're setting it zero for "
          << "safety. Consider using more robust solver such as PGS as a "
          << "secondary solver. If this happens even with PGS solver, please "
          << "report this as a bug.\n";
    mX.setZero();
  }

  // Print LCP formulation
  /*
  dtdbg << "After solve:" << std::endl;
  print(
      n,
      mA.data(),
      mX.data(),
      mLo.data(),
      mHi.data(),
      mB.data(),
      mW.data(),
      mFIndex.data());
  std::cout << std::endl;
  */

  // Clean up the results, this will clean up the mX vector to remove obvious
  // blemishes on the clamping indices
  /*
  LCPUtils::cleanUpResults(
      aGradientBackup,
      mX,
      bGradientBackup,
      hiGradientBackup,
      loGradientBackup,
      fIndexGradientBackup);
  */

  // If our short circuit didn't work, then we had to use the full LCP to get a
  // fresh solution, and now we have to generate new constraint matrices.
  if (group.getGradientConstraintMatrices() && !shortCircuitLCP)
  {
    group.getGradientConstraintMatrices()->registerLCPResults(
        mX,
        hiGradientBackup,
        loGradientBackup,
        fIndexGradientBackup,
        bGradientBackup,
        aColNormGradientBackup,
        aGradientBackup);
    group.getGradientConstraintMatrices()->constructMatrices(world);
    // If possible (if A is rank-deficient), change to an equivalent
    // least-squares solution that also satisfies the LCP
    group.getGradientConstraintMatrices()->opportunisticallyStandardizeResults(
        world, mX);
  }

  // Apply constraint impulses
  for (std::size_t i = 0; i < numConstraints; ++i)
  {
    const ConstraintBasePtr& constraint = group.getConstraint(i);
    constraint->applyImpulse(mX.data() + mOffset[i]);
    constraint->excite();
  }
}

//==============================================================================
#ifndef NDEBUG
bool BoxedLcpConstraintSolver::isSymmetric(std::size_t n, double* A)
{
  std::size_t nSkip = dPAD(n);
  for (std::size_t i = 0; i < n; ++i)
  {
    for (std::size_t j = 0; j < n; ++j)
    {
      if (std::abs(A[nSkip * i + j] - A[nSkip * j + i]) > 1e-6)
      {
        std::cout << "A: " << std::endl;
        for (std::size_t k = 0; k < n; ++k)
        {
          for (std::size_t l = 0; l < nSkip; ++l)
          {
            std::cout << std::setprecision(4) << A[k * nSkip + l] << " ";
          }
          std::cout << std::endl;
        }

        std::cout << "A(" << i << ", " << j << "): " << A[nSkip * i + j]
                  << std::endl;
        std::cout << "A(" << j << ", " << i << "): " << A[nSkip * j + i]
                  << std::endl;
        return false;
      }
    }
  }

  return true;
}

//==============================================================================
bool BoxedLcpConstraintSolver::isSymmetric(
    std::size_t n, double* A, std::size_t begin, std::size_t end)
{
  std::size_t nSkip = dPAD(n);
  for (std::size_t i = begin; i <= end; ++i)
  {
    for (std::size_t j = begin; j <= end; ++j)
    {
      if (std::abs(A[nSkip * i + j] - A[nSkip * j + i]) > 1e-6)
      {
        std::cout << "A: " << std::endl;
        for (std::size_t k = 0; k < n; ++k)
        {
          for (std::size_t l = 0; l < nSkip; ++l)
          {
            std::cout << std::setprecision(4) << A[k * nSkip + l] << " ";
          }
          std::cout << std::endl;
        }

        std::cout << "A(" << i << ", " << j << "): " << A[nSkip * i + j]
                  << std::endl;
        std::cout << "A(" << j << ", " << i << "): " << A[nSkip * j + i]
                  << std::endl;
        return false;
      }
    }
  }

  return true;
}

//==============================================================================
void BoxedLcpConstraintSolver::print(
    std::size_t n,
    double* A,
    double* x,
    double* /*lo*/,
    double* /*hi*/,
    double* b,
    double* w,
    int* findex)
{
  std::size_t nSkip = dPAD(n);
  std::cout << "A: " << std::endl;
  for (std::size_t i = 0; i < n; ++i)
  {
    for (std::size_t j = 0; j < nSkip; ++j)
    {
      std::cout << std::setprecision(4) << A[i * nSkip + j] << " ";
    }
    std::cout << std::endl;
  }

  std::cout << "b: ";
  for (std::size_t i = 0; i < n; ++i)
  {
    std::cout << std::setprecision(4) << b[i] << " ";
  }
  std::cout << std::endl;

  std::cout << "w: ";
  for (std::size_t i = 0; i < n; ++i)
  {
    std::cout << w[i] << " ";
  }
  std::cout << std::endl;

  std::cout << "x: ";
  for (std::size_t i = 0; i < n; ++i)
  {
    std::cout << x[i] << " ";
  }
  std::cout << std::endl;

  //  std::cout << "lb: ";
  //  for (int i = 0; i < dim; ++i)
  //  {
  //    std::cout << lb[i] << " ";
  //  }
  //  std::cout << std::endl;

  //  std::cout << "ub: ";
  //  for (int i = 0; i < dim; ++i)
  //  {
  //    std::cout << ub[i] << " ";
  //  }
  //  std::cout << std::endl;

  std::cout << "frictionIndex: ";
  for (std::size_t i = 0; i < n; ++i)
  {
    std::cout << findex[i] << " ";
  }
  std::cout << std::endl;

  double* Ax = new double[n];

  for (std::size_t i = 0; i < n; ++i)
  {
    Ax[i] = 0.0;
  }

  for (std::size_t i = 0; i < n; ++i)
  {
    for (std::size_t j = 0; j < n; ++j)
    {
      Ax[i] += A[i * nSkip + j] * x[j];
    }
  }

  std::cout << "Ax   : ";
  for (std::size_t i = 0; i < n; ++i)
  {
    std::cout << Ax[i] << " ";
  }
  std::cout << std::endl;

  std::cout << "b + w: ";
  for (std::size_t i = 0; i < n; ++i)
  {
    std::cout << b[i] + w[i] << " ";
  }
  std::cout << std::endl;

  delete[] Ax;
}
#endif

} // namespace constraint
} // namespace dart
