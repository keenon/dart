#include "dart/trajectory/LossFn.hpp"

#include "dart/utils/tl_optional.hpp"

using namespace dart;

namespace dart {
namespace trajectory {

//==============================================================================
LossFn::LossFn()
  : mLoss(tl::nullopt),
    mLossAndGrad(tl::nullopt),
    mLowerBound(-std::numeric_limits<double>::infinity()),
    mUpperBound(std::numeric_limits<double>::infinity())
{
}

//==============================================================================
LossFn::LossFn(TrajectoryLossFn loss)
  : mLoss(loss),
    mLossAndGrad(tl::nullopt),
    mLowerBound(-std::numeric_limits<double>::infinity()),
    mUpperBound(std::numeric_limits<double>::infinity())
{
}

//==============================================================================
LossFn::LossFn(TrajectoryLossFn loss, TrajectoryLossFnAndGrad lossAndGrad)
  : mLoss(loss),
    mLossAndGrad(lossAndGrad),
    mLowerBound(-std::numeric_limits<double>::infinity()),
    mUpperBound(std::numeric_limits<double>::infinity())
{
}

//==============================================================================
LossFn::~LossFn()
{
}

//==============================================================================
double LossFn::getLoss(const TrajectoryRollout* rollout)
{
  if (mLoss)
  {
    return mLoss.value()(rollout);
  }
  // Default to 0
  return 0.0;
}

//==============================================================================
double LossFn::getLossAndGradient(
    const TrajectoryRollout* rollout,
    /* OUT */ TrajectoryRollout* gradWrtRollout)
{
  if (mLossAndGrad)
  {
    return mLossAndGrad.value()(rollout, gradWrtRollout);
  }
  else if (mLoss)
  {
    TrajectoryRolloutReal rolloutCopy = TrajectoryRolloutReal(rollout);
    double originalLoss = mLoss.value()(&rolloutCopy);

    const double EPS = 1e-7;

    for (std::string key : rolloutCopy.getMappings())
    {
      for (int row = 0; row < rolloutCopy.getPoses(key).rows(); row++)
      {
        for (int col = 0; col < rolloutCopy.getPoses(key).cols(); col++)
        {
          rolloutCopy.getPoses(key)(row, col) += EPS;
          double lossPos = mLoss.value()(&rolloutCopy);
          rolloutCopy.getPoses(key)(row, col) -= EPS;
          gradWrtRollout->getPoses(key)(row, col)
              = (lossPos - originalLoss) / EPS;
        }
      }
      for (int row = 0; row < rolloutCopy.getVels(key).rows(); row++)
      {
        for (int col = 0; col < rolloutCopy.getVels(key).cols(); col++)
        {
          rolloutCopy.getVels(key)(row, col) += EPS;
          double lossVel = mLoss.value()(&rolloutCopy);
          rolloutCopy.getVels(key)(row, col) -= EPS;
          gradWrtRollout->getVels(key)(row, col)
              = (lossVel - originalLoss) / EPS;
        }
      }
      for (int row = 0; row < rolloutCopy.getForces(key).rows(); row++)
      {
        for (int col = 0; col < rolloutCopy.getForces(key).cols(); col++)
        {
          rolloutCopy.getForces(key)(row, col) += EPS;
          double lossForce = mLoss.value()(&rolloutCopy);
          rolloutCopy.getForces(key)(row, col) -= EPS;
          gradWrtRollout->getForces(key)(row, col)
              = (lossForce - originalLoss) / EPS;
        }
      }
    }

    return originalLoss;
  }

  // Default to 0
  for (std::string key : gradWrtRollout->getMappings())
  {
    gradWrtRollout->getPoses(key).setZero();
    gradWrtRollout->getVels(key).setZero();
    gradWrtRollout->getForces(key).setZero();
  }
  return 0.0;
}

//==============================================================================
/// If this LossFn is being used as a constraint, this gets the lower bound
/// it's allowed to reach
double LossFn::getLowerBound() const
{
  return mLowerBound;
}

//==============================================================================
/// If this LossFn is being used as a constraint, this sets the lower bound
/// it's allowed to reach
void LossFn::setLowerBound(double lowerBound)
{
  mLowerBound = lowerBound;
}

//==============================================================================
/// If this LossFn is being used as a constraint, this gets the upper bound
/// it's allowed to reach
double LossFn::getUpperBound() const
{
  return mUpperBound;
}

//==============================================================================
/// If this LossFn is being used as a constraint, this sets the upper bound
/// it's allowed to reach
void LossFn::setUpperBound(double upperBound)
{
  mUpperBound = upperBound;
}

} // namespace trajectory
} // namespace dart