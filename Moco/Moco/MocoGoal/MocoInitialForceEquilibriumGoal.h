#ifndef MOCO_MOCOINITIALFORCEEQUILIBRIUMGOAL_H
#define MOCO_MOCOINITIALFORCEEQUILIBRIUMGOAL_H
/* -------------------------------------------------------------------------- *
 * OpenSim Moco: MocoInitialForceEquilibriumGoal.h                            *
 * -------------------------------------------------------------------------- *
 * Copyright (c) 2019 Stanford University and the Authors                     *
 *                                                                            *
 * Author(s): Nicholas Bianco                                                 *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may    *
 * not use this file except in compliance with the License. You may obtain a  *
 * copy of the License at http://www.apache.org/licenses/LICENSE-2.0          *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 * -------------------------------------------------------------------------- */

#include "MocoGoal.h"

namespace OpenSim {

/// For all Muscle components with explicit tendon compliance dynamics, 
/// constrain (or minimize) the error computed from the muscle-tendon force 
/// equilibrium equation.
/// This goal ensures that the initial normalized tendon force state variable
/// is chosen such that equilibrium is satisfied; otherwise, the initial state
/// may not be valid.
/// This is an endpoint constraint goal by default.
/// @ingroup mocogoal
class OSIMMOCO_API MocoInitialForceEquilibriumGoal : public MocoGoal {
    OpenSim_DECLARE_CONCRETE_OBJECT(MocoInitialForceEquilibriumGoal, MocoGoal);

public:
    MocoInitialForceEquilibriumGoal() = default;
    MocoInitialForceEquilibriumGoal(std::string name)
        : MocoGoal(std::move(name)) {}

protected:
    bool getSupportsEndpointConstraintImpl() const override { return true; }
    Mode getDefaultModeImpl() const override {
        return Mode::EndpointConstraint;
    }
    /// Save references to muscles with tendon compliance enabled. If the muscle
    /// is of type DeGrooteFregly2016Muscle, check that tendon compliance
    /// dynamics mode is 'explicit'.
    void initializeOnModelImpl(const Model&) const override;
    void calcGoalImpl(
        const GoalInput& input, SimTK::Vector& goal) const override;

private:
    mutable std::vector<SimTK::ReferencePtr<const Muscle>>
        m_muscleRefs;
};

} // namespace OpenSim

#endif // MOCO_MOCOINITIALFORCEEQUILIBRIUMGOAL_H
