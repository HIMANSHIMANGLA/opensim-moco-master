/* -------------------------------------------------------------------------- *
 * OpenSim Moco: MocoStudy.cpp                                                *
 * -------------------------------------------------------------------------- *
 * Copyright (c) 2017 Stanford University and the Authors                     *
 *                                                                            *
 * Author(s): Christopher Dembia                                              *
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
#include "MocoStudy.h"

#include "Components/PositionMotion.h"
#include "MocoCasADiSolver/MocoCasADiSolver.h"
#include "MocoProblem.h"
#include "MocoTropterSolver.h"
#include "MocoUtilities.h"
#include <regex>

#include <OpenSim/Common/IO.h>
#include <OpenSim/Common/Reporter.h>
#include <OpenSim/Simulation/StatesTrajectory.h>

using namespace OpenSim;

MocoStudy::MocoStudy() { constructProperties(); }

MocoStudy::MocoStudy(const std::string& omocoFile) : Object(omocoFile) {
    constructProperties();
    updateFromXMLDocument();
}

void MocoStudy::constructProperties() {
    constructProperty_write_solution("./");
    constructProperty_problem(MocoProblem());
    constructProperty_solver(MocoCasADiSolver());
}

const MocoProblem& MocoStudy::getProblem() const { return get_problem(); }

MocoProblem& MocoStudy::updProblem() { return upd_problem(); }

MocoSolver& MocoStudy::initSolverInternal() {
    // TODO what to do if we already have a solver (from cloning?)
    upd_solver().resetProblem(get_problem());
    return upd_solver();
}

template <>
MocoTropterSolver& MocoStudy::initSolver<MocoTropterSolver>() {
    set_solver(MocoTropterSolver());
    return dynamic_cast<MocoTropterSolver&>(initSolverInternal());
}

template <>
MocoCasADiSolver& MocoStudy::initSolver<MocoCasADiSolver>() {
    set_solver(MocoCasADiSolver());
    return dynamic_cast<MocoCasADiSolver&>(initSolverInternal());
}

MocoTropterSolver& MocoStudy::initTropterSolver() {
    set_solver(MocoTropterSolver());
    return initSolver<MocoTropterSolver>();
}

MocoCasADiSolver& MocoStudy::initCasADiSolver() {
    return initSolver<MocoCasADiSolver>();
}

MocoSolver& MocoStudy::updSolver() { return updSolver<MocoSolver>(); }

MocoSolution MocoStudy::solve() const {
    // TODO avoid const_cast.
    const_cast<Self*>(this)->initSolverInternal();

    MocoSolution solution = get_solver().solve();

    bool originallySealed = solution.isSealed();
    if (get_write_solution() != "false") {
        OpenSim::IO::makeDir(get_write_solution());
        std::string prefix = getName().empty() ? "MocoStudy" : getName();
        solution.unseal();
        const std::string filename = get_write_solution() +
                                     SimTK::Pathname::getPathSeparator() +
                                     prefix + "_solution.sto";
        try {
            solution.write(filename);
        } catch (const TimestampGreaterThanEqualToNext&) {
            log_warn("Could not write solution to file...skipping.");
        }
        if (originallySealed) solution.seal();
    }
    return solution;
}

void MocoStudy::visualize(const MocoTrajectory& it) const {
    // TODO this does not need the Solver at all, so this could be moved to
    // MocoProblem.
    const auto& model = get_problem().getPhase(0).getModelProcessor().process();
    OpenSim::visualize(model, it.exportToStatesStorage());
}

TimeSeriesTable MocoStudy::analyze(const MocoTrajectory& trajectory,
        std::vector<std::string> outputPaths) const {
    return OpenSim::analyze<double>(get_problem().createRep().getModelBase(),
        trajectory, outputPaths);
}
