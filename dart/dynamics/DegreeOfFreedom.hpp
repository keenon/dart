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

#ifndef DART_DYNAMICS_DEGREEOFFREEDOM_HPP_
#define DART_DYNAMICS_DEGREEOFFREEDOM_HPP_

#include <memory>
#include <string>

#include <Eigen/Core>

#include "dart/common/Subject.hpp"
#include "dart/dynamics/SmartPointer.hpp"
#include "dart/math/MathTypes.hpp"

namespace dart {
namespace dynamics {

class Skeleton;
class Joint;
class BodyNode;

/// DegreeOfFreedom class is a proxy class for accessing single degrees of
/// freedom (aka generalized coordinates) of the Skeleton.
class DegreeOfFreedom : public virtual common::Subject
{
public:
  friend class Joint;
  template <class>
  friend class GenericJoint;
  friend class Skeleton;

  DegreeOfFreedom(const DegreeOfFreedom&) = delete;

  /// Change the name of this DegreeOfFreedom
  ///
  /// The _preserveName argument will be passed to the preserveName(bool)
  /// function. Set _preserveName to true when customizing the name of the
  /// DegreeOfFreedom; that way the name will not be overwritten if the Joint
  /// name changes.
  const std::string& setName(
      const std::string& _name, bool _preserveName = true);

  /// \brief Get the name of this DegreeOfFreedom
  ///
  /// DegreeOfFreedom's name will be automatically given by the joint it belongs
  /// to. Below is the naming policy:
  ///   - GenericJoint<RealSpace> \n
  ///       Same name as the joint it belongs to.
  ///   - GenericJoint<[multi_dof_joint_space]> \n
  ///       "[Joint_name]+[affix]" is used. The affix is determined according
  ///       to the role they play in the joint. For example, suppose there's a
  ///       TranslationalJoint named "trans_joint". Then the each dof to be
  ///       named "trans_joint_x", "trans_joint_y", and "trans_joint_z".
  ///   - ZeroDofJoint \n
  ///       ZeroDofJoint doesn't have dof.
  ///
  /// The default name can be renamed by setName() as well.
  const std::string& getName() const;

  /// Prevent Joint::updateDegreeOfFreedomNames() from changing the name of this
  /// degree of freedom. This is useful if you (the user) have customized the
  /// name for this DegreeOfFreedom and want to prevent DART from automatically
  /// updating its name if its parent Joint properties ever change.
  void preserveName(bool _preserve);

  /// Check whether DegreeOfFreedom::lockName(bool) is activate
  bool isNamePreserved() const;

  /// Get this DegreeOfFreedom's index within its Skeleton
  std::size_t getIndexInSkeleton() const;

  /// Get this DegreeOfFreedom's index within its tree
  std::size_t getIndexInTree() const;

  /// Get this DegreeOfFreedom's index within its Joint
  std::size_t getIndexInJoint() const;

  /// Get the index of the tree that this DegreeOfFreedom belongs to
  std::size_t getTreeIndex() const;

  //----------------------------------------------------------------------------
  /// \{ \name Command
  //----------------------------------------------------------------------------

  /// Set the command of this DegreeOfFreedom
  void setCommand(s_t _command);

  /// Get the command of this DegreeOfFreedom
  s_t getCommand() const;

  /// Set the command of this DegreeOfFreedom to zero
  void resetCommand();

  /// \}

  //----------------------------------------------------------------------------
  /// \{ \name Position
  //----------------------------------------------------------------------------

  /// Set the position of this DegreeOfFreedom
  void setPosition(s_t _position);

  /// Get the position of this DegreeOfFreedom
  s_t getPosition() const;

  /// Set the position limits of this DegreeOfFreedom
  void setPositionLimits(s_t _lowerLimit, s_t _upperLimit);

  /// Set the position limits of this DegreeOfFreedom
  void setPositionLimits(const std::pair<s_t, s_t>& _limits);

  /// Get the position limits of this DegreeOfFreedom
  std::pair<s_t, s_t> getPositionLimits() const;

  /// Set the lower position limit of this DegreeOfFreedom
  void setPositionLowerLimit(s_t _limit);

  /// Get the lower position limit of this DegreeOfFreedom
  s_t getPositionLowerLimit() const;

  /// Set the upper position limit of this DegreeOfFreedom
  void setPositionUpperLimit(s_t _limit);

  /// Get the upper position limit of this DegreeOfFreedom
  s_t getPositionUpperLimit() const;

  /// Get whether this DOF is cyclic. Return true if and only if an infinite
  /// number of DOF positions produce the same local transform. If this DOF is
  /// part of a multi-DOF joint, then producing a cycle may require altering
  /// the position of the Joint's other DOFs.
  bool isCyclic() const;

  /// Get whether the position of this DegreeOfFreedom has limits.
  bool hasPositionLimit() const;

  /// Set the position of this DegreeOfFreedom to zero
  void resetPosition();

  /// Change the position that resetPosition() will give to this DegreeOfFreedom
  void setInitialPosition(s_t _initial);

  /// Get the position that resetPosition() will give to this DegreeOfFreedom
  s_t getInitialPosition() const;

  /// \}

  //----------------------------------------------------------------------------
  /// \{ \name Velocity
  //----------------------------------------------------------------------------

  /// Set the velocity of this DegreeOfFreedom
  void setVelocity(s_t _velocity);

  /// Get the velocity of this DegreeOfFreedom
  s_t getVelocity() const;

  /// Set the velocity limits of this DegreeOfFreedom
  void setVelocityLimits(s_t _lowerLimit, s_t _upperLimit);

  /// Set the velocity limtis of this DegreeOfFreedom
  void setVelocityLimits(const std::pair<s_t, s_t>& _limits);

  /// Get the velocity limits of this DegreeOfFreedom
  std::pair<s_t, s_t> getVelocityLimits() const;

  /// Set the lower velocity limit of this DegreeOfFreedom
  void setVelocityLowerLimit(s_t _limit);

  /// Get the lower velocity limit of this DegreeOfFreedom
  s_t getVelocityLowerLimit() const;

  /// Set the upper velocity limit of this DegreeOfFreedom
  void setVelocityUpperLimit(s_t _limit);

  /// Get the upper Velocity limit of this DegreeOfFreedom
  s_t getVelocityUpperLimit() const;

  /// Set the velocity of this DegreeOfFreedom to zero
  void resetVelocity();

  /// Change the velocity that resetVelocity() will give to this DegreeOfFreedom
  void setInitialVelocity(s_t _initial);

  /// Get the velocity that resetVelocity() will give to this DegreeOfFreedom
  s_t getInitialVelocity() const;

  /// \}

  //----------------------------------------------------------------------------
  /// \{ \name Acceleration
  //----------------------------------------------------------------------------

  /// Set the acceleration of this DegreeOfFreedom
  void setAcceleration(s_t _acceleration);

  /// Get the acceleration of this DegreeOfFreedom
  s_t getAcceleration() const;

  /// Set the acceleration of this DegreeOfFreedom to zero
  void resetAcceleration();

  /// Set the acceleration limits of this DegreeOfFreedom
  void setAccelerationLimits(s_t _lowerLimit, s_t _upperLimit);

  /// Set the acceleartion limits of this DegreeOfFreedom
  void setAccelerationLimits(const std::pair<s_t, s_t>& _limits);

  /// Get the acceleration limits of this DegreeOfFreedom
  std::pair<s_t, s_t> getAccelerationLimits() const;

  /// Set the lower acceleration limit of this DegreeOfFreedom
  void setAccelerationLowerLimit(s_t _limit);

  /// Get the lower acceleration limit of this DegreeOfFreedom
  s_t getAccelerationLowerLimit() const;

  /// Set the upper acceleration limit of this DegreeOfFreedom
  void setAccelerationUpperLimit(s_t _limit);

  /// Get the upper acceleration limit of this DegreeOfFreedom
  s_t getAccelerationUpperLimit() const;

  /// \}

  //----------------------------------------------------------------------------
  /// \{ \name Force
  //----------------------------------------------------------------------------

  /// Set the generalized force of this DegreeOfFreedom
  void setControlForce(s_t _force);

  /// Get the generalized force of this DegreeOfFreedom
  s_t getControlForce() const;

  /// Set the generalized force of this DegreeOfFreedom to zero
  void resetControlForce();

  /// Set the generalized force limits of this DegreeOfFreedom
  void setControlForceLimits(s_t _lowerLimit, s_t _upperLimit);

  /// Set the generalized force limits of this DegreeOfFreedom
  void setControlForceLimits(const std::pair<s_t, s_t>& _limits);

  /// Get the generalized force limits of this DegreeOfFreedom
  std::pair<s_t, s_t> getControlForceLimits() const;

  /// Set the lower generalized force limit of this DegreeOfFreedom
  void setControlForceLowerLimit(s_t _limit);

  /// Get the lower generalized force limit of this DegreeOfFreedom
  s_t getControlForceLowerLimit() const;

  /// Set the upper generalized force limit of this DegreeOfFreedom
  void setControlForceUpperLimit(s_t _limit);

  /// Get the upper generalized force limit of this DegreeOfFreedom
  s_t getControlForceUpperLimit() const;

  /// \}

  //----------------------------------------------------------------------------
  /// \{ \name Velocity change
  //----------------------------------------------------------------------------

  /// Set the velocity change of this DegreeOfFreedom
  void setVelocityChange(s_t _velocityChange);

  /// Get the velocity change of this DegreeOfFreedom
  s_t getVelocityChange() const;

  /// Set the velocity change of this DegreeOfFreedom to zero
  void resetVelocityChange();

  /// \}

  //----------------------------------------------------------------------------
  /// \{ \name Constraint impulse
  //----------------------------------------------------------------------------

  /// Set the constraint impulse of this generalized coordinate
  void setConstraintImpulse(s_t _impulse);

  /// Get the constraint impulse of this generalized coordinate
  s_t getConstraintImpulse() const;

  /// Set the constraint impulse of this generalized coordinate to zero
  void resetConstraintImpulse();

  /// \}

  //----------------------------------------------------------------------------
  /// \{ \name Passive forces - spring, viscous friction, Coulomb friction
  //----------------------------------------------------------------------------

  /// Set stiffness of the spring force for this generalized coordinate
  void setSpringStiffness(s_t _k);

  /// Get stiffness of the spring force for this generalized coordinate
  s_t getSpringStiffness() const;

  /// Set rest position for the spring force of this generalized coordinate
  void setRestPosition(s_t _q0);

  /// Get rest position for the spring force of this generalized coordinate
  s_t getRestPosition() const;

  /// Set coefficient of damping (viscous friction) force for this generalized
  /// coordinate
  void setDampingCoefficient(s_t _coeff);

  /// Get coefficient of damping (viscous friction) force for this generalized
  /// coordinate
  s_t getDampingCoefficient() const;

  /// Set Coulomb friction force for this generalized coordinate
  void setCoulombFriction(s_t _friction);

  /// Get Coulomb friction force for this generalized coordinate
  s_t getCoulombFriction() const;

  /// \}

  //----------------------------------------------------------------------------
  /// \{ \name Relationships
  //----------------------------------------------------------------------------

  /// Get the Joint that this DegreeOfFreedom belongs to
  Joint* getJoint();

  /// Get the Joint that this DegreeOfFreedom belongs to
  const Joint* getJoint() const;

  /// Get the Skeleton that this DegreeOfFreedom is inside of
  SkeletonPtr getSkeleton();

  /// Get the Skeleton that this DegreeOfFreedom is inside of
  ConstSkeletonPtr getSkeleton() const;

  /// Get the BodyNode downstream of this DegreeOfFreedom
  BodyNode* getChildBodyNode();

  /// Get the BodyNode downstream of this DegreeOfFreedom
  const BodyNode* getChildBodyNode() const;

  /// Get the BodyNode upstream of this DegreeOfFreedom
  BodyNode* getParentBodyNode();

  /// Get the BodyNode upstream of this DegreeOfFreedom
  const BodyNode* getParentBodyNode() const;

  /// This does a slow walk up the tree to figure out if `target` is a child of
  /// this node.
  bool isParentOf(const DegreeOfFreedom* target) const;

  /// This does a slow walk up the tree to figure out if `target` is a child of
  /// this node.
  bool isParentOf(const BodyNode* target) const;

  /// This uses the cached version, stored on the parent Skeleton, to return the
  /// same value as isParentOf()
  bool isParentOfFast(const DegreeOfFreedom* target) const;

  /// This uses the cached version, stored on the parent Skeleton, to return the
  /// same value as isParentOf()
  bool isParentOfFast(const BodyNode* target) const;

  /// \}

protected:
  /// The constructor is protected so that only Joints can create
  /// DegreeOfFreedom classes
  DegreeOfFreedom(Joint* _joint, std::size_t _indexInJoint);

  /// \brief Index of this DegreeOfFreedom within its Joint
  ///
  /// The index is determined when this DegreeOfFreedom is created by the Joint
  /// it belongs to. Note that the index should be unique within the Joint.
  std::size_t mIndexInJoint;

  /// Index of this DegreeOfFreedom within its Skeleton
  std::size_t mIndexInSkeleton;

  /// Index of this DegreeOfFreedom within its tree
  std::size_t mIndexInTree;

  /// The joint that this DegreeOfFreedom belongs to
  Joint* mJoint;
  // Note that we do not need to store BodyNode or Skeleton, because we can
  // access them through this joint pointer. Moreover, we never need to check
  // whether mJoint is nullptr, because only Joints are allowed to create a
  // DegreeOfFreedom and every DegreeOfFreedom is deleted when its Joint is
  // destructed.
};

} // namespace dynamics
} // namespace dart

#endif // DART_DYNAMICS_DEGREEOFFREEDOM_HPP_
