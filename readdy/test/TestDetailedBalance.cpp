/********************************************************************
 * Copyright © 2018 Computational Molecular Biology Group,          *
 *                  Freie Universität Berlin (GER)                  *
 *                                                                  *
 * This file is part of ReaDDy.                                     *
 *                                                                  *
 * ReaDDy is free software: you can redistribute it and/or modify   *
 * it under the terms of the GNU Lesser General Public License as   *
 * published by the Free Software Foundation, either version 3 of   *
 * the License, or (at your option) any later version.              *
 *                                                                  *
 * This program is distributed in the hope that it will be useful,  *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of   *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the     *
 * GNU Lesser General Public License for more details.              *
 *                                                                  *
 * You should have received a copy of the GNU Lesser General        *
 * Public License along with this program. If not, see              *
 * <http://www.gnu.org/licenses/>.                                  *
 ********************************************************************/


/**
 * @file TestDetailedBalance.cpp
 * @brief Kernel-non-specific tests of the detailed-balance reaction handler
 * @author chrisfroe
 * @date 29.05.18
 * @copyright GNU Lesser General Public License v3.0
 */

#include <gtest/gtest.h>
#include <readdy/common/common.h>
#include <readdy/testing/KernelTest.h>
#include <readdy/testing/Utils.h>
#include <readdy/model/actions/Action.h>

namespace {

class TestDetailedBalanceWithKernels : public KernelTest {

};

namespace m = readdy::model;

auto registerAbcSpecies = [](m::Context &ctx){
    ctx.particle_types().add("A", 1.);
    ctx.particle_types().add("B", 1.);
    ctx.particle_types().add("C", 1.);
};

TEST(TestDetailedBalance, ReversibleReactionConfigValues) {
    {
        m::Context ctx;
        ctx.boxSize() = {21.544346900318832, 21.544346900318832, 21.544346900318832};
        ctx.kBT() = 1;
        registerAbcSpecies(ctx);
        ctx.potentials().addHarmonicRepulsion("A", "B", 2., 3.);
        ctx.reactions().add("fusion: A +(5) B -> C", 1.);
        ctx.reactions().add("fission: C -> A +(5) B", 2.);
        ctx.configure();

        auto idFusion = ctx.reactions().idOf("fusion");
        auto idFission = ctx.reactions().idOf("fission");
        m::actions::reactions::ReversibleReactionConfig conf(idFusion, idFission, ctx);
        EXPECT_NEAR(conf.totalVolume, 10000, 1e-7);
        EXPECT_FLOAT_EQ(conf.kbt, 1.);
        EXPECT_FLOAT_EQ(conf.lhsInteractionRadius, 3.);
        EXPECT_FLOAT_EQ(conf.reactionRadius, 5.);
        EXPECT_EQ(conf.rhsTypes[0], ctx.particle_types().idOf("C"));
        EXPECT_EQ(conf.lhsTypes[0], ctx.particle_types().idOf("A"));
        EXPECT_EQ(conf.lhsTypes[1], ctx.particle_types().idOf("B"));
        EXPECT_NEAR(conf.lhsInteractionVolume, 113.09733552923254, 1e-6);
        EXPECT_NEAR(conf.effectiveLhsInteractionVolume, 68.099109181202977, 1e-6);
        EXPECT_NEAR(conf.effectiveLhsReactionVolume, 478.6005724, 1e-6);
        EXPECT_NEAR(conf.equilibriumConstant, 41.6004591207713 , 1e-6);
        EXPECT_FLOAT_EQ(conf.macroBackwardRate, 2.);
        EXPECT_NEAR(conf.macroForwardRate, 2. / 41.6004591207713 * 10000., 1e-6);
    }

    // @todo ConversionConversion
    // @todo EnzymaticEnzymatic
}

TEST(TestDetailedBalance, ReversibleReactionConfigFalseInput) {
    EXPECT_TRUE(false);
    // @todo Enzymatic with non-matching reaction radii
    // @todo pair of non reversible reactions
}

TEST(TestDetailedBalance, SearchReversibleReactions) {
    {
        m::Context ctx;
        registerAbcSpecies(ctx);
        // @todo
    }

}


auto abcFusionFissionContext = [](readdy::model::Context &ctx, readdy::scalar rateOn, readdy::scalar rateOff){
    ctx.boxSize() = {{10, 10, 10}};
    ctx.particle_types().add("A", 1.);
    ctx.particle_types().add("B", 1.);
    ctx.particle_types().add("C", 1.);
    readdy::scalar reactionRadius = 2.;
    ctx.potentials().addHarmonicRepulsion("A", "B", 10., reactionRadius);
    ctx.potentials().addHarmonicRepulsion("B", "B", 10., reactionRadius);
    ctx.potentials().addHarmonicRepulsion("A", "A", 10., reactionRadius);
    ctx.potentials().addHarmonicRepulsion("A", "C", 10., 1+1.2599210498948732);
    ctx.potentials().addHarmonicRepulsion("B", "C", 10., 1+1.2599210498948732);
    ctx.potentials().addHarmonicRepulsion("C", "C", 10., 2.*1.2599210498948732);
    ctx.reactions().addFission("fission", "C", "A", "B", rateOff, reactionRadius);
    ctx.reactions().addFusion("fusion", "A", "B", "C", rateOn, reactionRadius);
    ctx.configure();
};

auto perform = [](readdy::model::Kernel *kernel, size_t nSteps, readdy::scalar timeStep) {
    auto &&integrator = kernel->actions().eulerBDIntegrator(timeStep);
    auto &&forces = kernel->actions().calculateForces();
    using update_nl = readdy::model::actions::UpdateNeighborList;
    auto &&initNeighborList = kernel->actions().updateNeighborList(update_nl::Operation::init, 0);
    auto &&neighborList = kernel->actions().updateNeighborList(update_nl::Operation::update, 0);
    auto &&reactions = kernel->actions().detailedBalance(timeStep);

    initNeighborList->perform();
    neighborList->perform();
    forces->perform();
    kernel->evaluateObservables(0);
    for (size_t t = 1; t < nSteps+1; t++) {
        integrator->perform();
        neighborList->perform();
        forces->perform();
        reactions->perform();
        neighborList->perform();
        forces->perform();
        kernel->evaluateObservables(t);
    }
};

TEST_P(TestDetailedBalanceWithKernels, FusionThatShouldBeRejected) {
    auto &ctx = kernel->context();
    abcFusionFissionContext(ctx, 1e9, 1.); // high on rate

    const auto idfus = ctx.reactions().idOf("fusion");
    const auto idfis = ctx.reactions().idOf("fission");

    auto countsObs = kernel->observe().reactionCounts(1);
    countsObs->setCallback([&idfus, &idfis](const readdy::model::observables::ReactionCounts::result_type &result) {
        if (result.empty()) {
            readdy::log::trace("reaction counts is empty, no reaction handler ran so far, skip test");
            return;
        }
        EXPECT_EQ(result.at(idfus), 0) << "fusion shall not occur because it should be rejected";
        EXPECT_EQ(result.at(idfis), 0) << "fission shall not occur because only one timestep "
                                          "is performed in which a fusion is rejected";
    });
    auto countsConnection = kernel->connectObservable(countsObs.get());

    std::vector<std::string> typesToCount = {"A", "B", "C"};
    auto numbersObs = kernel->observe().nParticles(1, typesToCount);
    numbersObs->setCallback([](const readdy::model::observables::NParticles::result_type &result) {
        EXPECT_EQ(result[0]+result[2], 1) << "conservation of A + C";
        EXPECT_EQ(result[1]+result[2], 1) << "conservation of B + C";
        if (result[0]+result[2] != 1) {
            readdy::log::trace("A {} B {} C {}", result[0], result[1], result[2]);
        }
    });
    auto numbersConnection = kernel->connectObservable(numbersObs.get());

    const auto ida = ctx.particle_types().idOf("A");
    const auto idb = ctx.particle_types().idOf("B");
    kernel->stateModel().addParticle({{0.1, 0.1, 0.1}, ida});
    kernel->stateModel().addParticle({{-0.1, -0.1, -0.1}, idb});

    // @todo induce rejection by many repulsing particles in the vicinity
    // add many C particles within reactionRadius around the created C particle (0,0,0)

    readdy::scalar timeStep = 0.1;
    perform(kernel.get(), 1, timeStep);
}

TEST_P(TestDetailedBalanceWithKernels, FissionThatShouldBeRejected) {
    auto &ctx = kernel->context();
    abcFusionFissionContext(ctx, 1., 1e9); // very high off rate

    const auto idfus = ctx.reactions().idOf("fusion");
    const auto idfis = ctx.reactions().idOf("fission");

    auto countsObs = kernel->observe().reactionCounts(1);
    countsObs->setCallback([&idfus, &idfis](const readdy::model::observables::ReactionCounts::result_type &result) {
        if (result.empty()) {
            readdy::log::trace("reaction counts is empty, no reaction handler ran so far, skip test");
            return;
        }
        EXPECT_EQ(result.at(idfus), 0) << "fidsion shall not occur because it should be rejected";
        EXPECT_EQ(result.at(idfis), 0) << "fusion shall not occur because only one timestep "
                                          "is performed in which a fission is rejected";
    });
    auto countsConnection = kernel->connectObservable(countsObs.get());

    std::vector<std::string> typesToCount = {"A", "B", "C"};
    auto numbersObs = kernel->observe().nParticles(1, typesToCount);
    numbersObs->setCallback([](const readdy::model::observables::NParticles::result_type &result) {
        EXPECT_EQ(result[0]+result[2], 1) << "conservation of A + C";
        EXPECT_EQ(result[1]+result[2], 1) << "conservation of B + C";
        if (result[0]+result[2] != 1) {
            readdy::log::trace("A {} B {} C {}", result[0], result[1], result[2]);
        }
    });
    auto numbersConnection = kernel->connectObservable(numbersObs.get());

    const auto idc = ctx.particle_types().idOf("C");
    kernel->stateModel().addParticle({{-0.1, -0.1, -0.1}, idc});

    // @todo induce rejection by many repulsing particles in the vicinity
    // add many A particles within reactionRadius of the C particle

    readdy::scalar timeStep = 0.1;
    perform(kernel.get(), 1, timeStep);
}

TEST_P(TestDetailedBalanceWithKernels, ConservationOfParticles) {
    auto &ctx = kernel->context();
    abcFusionFissionContext(ctx, 10., 2.);

    readdy::scalar dissociationConstant = 10.; // @todo remove

    std::vector<std::string> typesToCount = {"A", "B", "C"};
    auto numbersObs = kernel->observe().nParticles(1, typesToCount);
    numbersObs->setCallback([](const readdy::model::observables::NParticles::result_type &result) {
        EXPECT_EQ(result[0]+result[2], 20) << "conservation of A + C";
        EXPECT_EQ(result[1]+result[2], 20) << "conservation of B + C";
        if (result[0]+result[2] != 1) {
            readdy::log::trace("A {} B {} C {}", result[0], result[1], result[2]);
        }
    });
    auto numbersConnection = kernel->connectObservable(numbersObs.get());

    const auto ida = ctx.particle_types().idOf("A");
    const auto idb = ctx.particle_types().idOf("B");
    const int n_particles = 20;
    std::vector<readdy::model::Particle> particlesA;
    std::vector<readdy::model::Particle> particlesB;
    for (auto i=0; i<n_particles; ++i) {
        particlesA.emplace_back(readdy::model::rnd::normal3<readdy::scalar>(), ida);
        particlesB.emplace_back(readdy::model::rnd::normal3<readdy::scalar>(), idb);
    }
    kernel->stateModel().addParticles(particlesA);
    kernel->stateModel().addParticles(particlesB);

    readdy::scalar timeStep = 0.01;
    perform(kernel.get(), 100, timeStep);
}

INSTANTIATE_TEST_CASE_P(TestDetailedBalanceWithKernels, TestDetailedBalanceWithKernels,
                        ::testing::ValuesIn(readdy::testing::getKernelsToTest()));

}