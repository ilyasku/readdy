/********************************************************************
 * Copyright © 2019 Computational Molecular Biology Group,          *
 *                  Freie Universität Berlin (GER)                  *
 *                                                                  *
 * Redistribution and use in source and binary forms, with or       *
 * without modification, are permitted provided that the            *
 * following conditions are met:                                    *
 *  1. Redistributions of source code must retain the above         *
 *     copyright notice, this list of conditions and the            *
 *     following disclaimer.                                        *
 *  2. Redistributions in binary form must reproduce the above      *
 *     copyright notice, this list of conditions and the following  *
 *     disclaimer in the documentation and/or other materials       *
 *     provided with the distribution.                              *
 *  3. Neither the name of the copyright holder nor the names of    *
 *     its contributors may be used to endorse or promote products  *
 *     derived from this software without specific                  *
 *     prior written permission.                                    *
 *                                                                  *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND           *
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,      *
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF         *
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE         *
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR            *
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,     *
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,         *
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; *
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER *
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,      *
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)    *
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF      *
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                       *
 ********************************************************************/


/**
 * @file TestBreakingBonds.cpp
 * @brief Test implementation-independent execution of action that breaks bonds
 * @author chrisfroe
 * @date 27.05.19
 */

#include <catch2/catch.hpp>

#include <readdy/testing/KernelTest.h>
#include <readdy/testing/Utils.h>

namespace m = readdy::model;
using namespace readdytesting::kernel;

TEMPLATE_TEST_CASE("Test breaking bonds.", "[breakbonds]", SingleCPU, CPU) {
    auto kernel = readdytesting::kernel::create<TestType>();
    auto &ctx = kernel->context();
    ctx.boxSize() = {10., 10., 10.};
    auto &types = ctx.particleTypes();
    auto &stateModel = kernel->stateModel();

    types.add("A", 1.0);
    types.add("B", 1.0);

    auto &topReg = ctx.topologyRegistry();
    topReg.addType("T");
    readdy::scalar timeStep = 1.;

    GIVEN("A dimer A-A with fixed positions and bond potential") {
        readdy::api::Bond bond{1., 1., readdy::api::BondType::HARMONIC};
        topReg.configureBondPotential("A", "A", bond);

        std::vector<readdy::model::TopologyParticle> particles{
                {0., 0., 0., types.idOf("A")},
                {0., 0., 2., types.idOf("A")}
        };
        // distance of A and A is 2, equilibrium distance is 1, bond extension is 1, bond energy is 1.

        auto graphTop = stateModel.addTopology(topReg.idOf("T"), particles);
        auto &graph = graphTop->graph();
        graph.addEdgeBetweenParticles(0, 1);

        // We have one dimer
        auto topsBefore = stateModel.getTopologies();
        REQUIRE(topsBefore.size() == 1);
        REQUIRE(topsBefore.at(0)->type() == topReg.idOf("T"));
        REQUIRE(topsBefore.at(0)->getNParticles() == 2);

        WHEN("the threshold for breaking is low") {
            readdy::model::actions::top::BreakConfig breakConfig;
            breakConfig.addBreakablePair(types.idOf("A"), types.idOf("A"), 0.9, 1e10);

            auto &&breakingBonds = kernel->actions().breakBonds(timeStep, breakConfig);
            breakingBonds->perform();

            THEN("the bond breaks and there are no topologies") {
                auto topsAfter = stateModel.getTopologies();
                //readdy::log::warn("topology[0] has size {}", topsAfter.at(0)->getNParticles());
                REQUIRE(topsAfter.empty());
            }
        }

        WHEN(" the threshold for breaking is too high") {
            readdy::model::actions::top::BreakConfig breakConfig;
            breakConfig.addBreakablePair(types.idOf("A"), types.idOf("A"), 1.1, 1e10);

            auto &&breakingBonds = kernel->actions().breakBonds(timeStep, breakConfig);
            breakingBonds->perform();

            THEN("the bond does not break and we still have one dimer") {
                auto topsAfter = stateModel.getTopologies();
                REQUIRE(topsAfter.size() == 1);
                REQUIRE(topsAfter.at(0)->type() == topReg.idOf("T"));
                REQUIRE(topsAfter.at(0)->getNParticles() == 2);
            }
        }

        // particles are still at their position
        auto ps = stateModel.getParticles();
        REQUIRE(ps.size() == 2);
        REQUIRE(ps.at(0).type() == types.idOf("A"));
        REQUIRE(ps.at(1).type() == types.idOf("A"));
        if (readdy::testing::vec3eq(ps.at(0).pos(), readdy::Vec3(0., 0., 0.))) {
            REQUIRE(readdy::testing::vec3eq(ps.at(1).pos(), readdy::Vec3(0., 0., 2.)));
        } else {
            REQUIRE(readdy::testing::vec3eq(ps.at(0).pos(), readdy::Vec3(0., 0., 2.)));
            REQUIRE(readdy::testing::vec3eq(ps.at(1).pos(), readdy::Vec3(0., 0., 0.)));
        }

    }

    GIVEN("A linear trimer AAB with fixed positions and bond potentials") {
        readdy::api::Bond bond{1., 1., readdy::api::BondType::HARMONIC};
        topReg.configureBondPotential("A", "A", bond);
        topReg.configureBondPotential("B", "A", bond);

        std::vector<readdy::model::TopologyParticle> particles{
                {0., 0., -1., types.idOf("A")},
                {0., 0., 1., types.idOf("A")},
                {0., 0., 3., types.idOf("B")}
        };
        // distance of A and A is 2, distance of A and B is 2

        auto graphTop = stateModel.addTopology(topReg.idOf("T"), particles);
        auto &graph = graphTop->graph();
        graph.addEdgeBetweenParticles(0, 1);
        graph.addEdgeBetweenParticles(1, 2);

        // We have one topology
        auto topsBefore = stateModel.getTopologies();
        REQUIRE(topsBefore.size() == 1);
        REQUIRE(topsBefore.at(0)->type() == topReg.idOf("T"));
        REQUIRE(topsBefore.at(0)->getNParticles() == 3);

        WHEN("A-B is a breakable bond with low threshold and high rate") {
            readdy::model::actions::top::BreakConfig breakConfig;
            breakConfig.addBreakablePair(types.idOf("A"), types.idOf("B"), 0.9, 1e10);

            THEN("the bond A-B breaks and we are left with a A-A topology") {
                auto &&breakingBonds = kernel->actions().breakBonds(timeStep, breakConfig);
                breakingBonds->perform();

                auto topsAfter = stateModel.getTopologies();
                REQUIRE(topsAfter.size() == 1);
                REQUIRE(topsAfter.at(0)->type() == topReg.idOf("T"));
                REQUIRE(topsAfter.at(0)->getNParticles() == 2);

                auto ps = topsAfter.at(0)->fetchParticles();
                REQUIRE(ps.size() == 2);
                REQUIRE(ps.at(0).type() == types.idOf("A"));
                REQUIRE(ps.at(1).type() == types.idOf("A"));
                if (readdy::testing::vec3eq(ps.at(0).pos(), readdy::Vec3(0., 0., -1.))) {
                    REQUIRE(readdy::testing::vec3eq(ps.at(1).pos(), readdy::Vec3(0., 0., 1.)));
                } else {
                    REQUIRE(readdy::testing::vec3eq(ps.at(1).pos(), readdy::Vec3(0., 0., -1.)));
                    REQUIRE(readdy::testing::vec3eq(ps.at(0).pos(), readdy::Vec3(0., 0., 1.)));
                }
            }
        }

        WHEN("A-A is a breakable bond with low threshold and high rate") {
            readdy::model::actions::top::BreakConfig breakConfig;
            breakConfig.addBreakablePair(types.idOf("A"), types.idOf("A"), 0.9, 1e10);

            THEN("the bond A-A breaks and we are left with a A-B topology") {
                auto &&breakingBonds = kernel->actions().breakBonds(timeStep, breakConfig);
                breakingBonds->perform();

                auto topsAfter = stateModel.getTopologies();
                REQUIRE(topsAfter.size() == 1);
                REQUIRE(topsAfter.at(0)->type() == topReg.idOf("T"));
                REQUIRE(topsAfter.at(0)->getNParticles() == 2);

                auto ps = topsAfter.at(0)->fetchParticles();
                REQUIRE(ps.size() == 2);

                bool oneOfEach =
                        (ps.at(0).type() == types.idOf("A") and ps.at(1).type() == types.idOf("B"))
                        or (ps.at(1).type() == types.idOf("A") and ps.at(0).type() == types.idOf("B"));
                REQUIRE(oneOfEach);
            }
        }

        WHEN("both bonds are in principle breakable but the threshold is too high") {
            readdy::model::actions::top::BreakConfig breakConfig;
            breakConfig.addBreakablePair(types.idOf("A"), types.idOf("A"), 1.1, 1e10);
            breakConfig.addBreakablePair(types.idOf("A"), types.idOf("B"), 1.1, 1e10);

            THEN("the trimer is unaffected") {
                auto &&breakingBonds = kernel->actions().breakBonds(timeStep, breakConfig);
                breakingBonds->perform();

                auto topsAfter = stateModel.getTopologies();
                REQUIRE(topsAfter.size() == 1);
                REQUIRE(topsAfter.at(0)->type() == topReg.idOf("T"));
                REQUIRE(topsAfter.at(0)->getNParticles() == 3);
            }
        }

        // all particles are still at their position
        auto ps = stateModel.getParticles();
        REQUIRE(ps.size() == 3);
        auto numberA = std::count_if(ps.begin(), ps.end(),
                                     [&types](const readdy::model::Particle &p) {
                                         return p.type() == types.idOf("A");
                                     });
        REQUIRE(numberA == 2);
        auto numberB = std::count_if(ps.begin(), ps.end(),
                                     [&types](const readdy::model::Particle &p) {
                                         return p.type() == types.idOf("B");
                                     });
        REQUIRE(numberB == 1);

        auto bParticleIt = std::find_if(ps.begin(), ps.end(),
                                        [&types](const readdy::model::Particle &p) {
                                            return p.type() == types.idOf("B");
                                        });
        REQUIRE(bParticleIt != ps.end());
        REQUIRE(readdy::testing::vec3eq(bParticleIt->pos(), readdy::Vec3(0., 0., 3.)));

        auto aParticle1 = std::find_if(ps.begin(), ps.end(),
                                       [&types](const readdy::model::Particle &p) {
                                           return p.type() == types.idOf("A");
                                       });
        REQUIRE(aParticle1 != ps.end());

        auto next = aParticle1;
        std::advance(next, 1);
        auto aParticle2 = std::find_if(next, ps.end(),
                                       [&types](const readdy::model::Particle &p) {
                                           return p.type() == types.idOf("A");
                                       });
        REQUIRE(aParticle2 != ps.end());

        if (readdy::testing::vec3eq(aParticle1->pos(), readdy::Vec3(0., 0., -1.))) {
            REQUIRE(readdy::testing::vec3eq(aParticle2->pos(), readdy::Vec3(0., 0., 1.)));
        } else {
            REQUIRE(readdy::testing::vec3eq(aParticle1->pos(), readdy::Vec3(0., 0., 1.)));
            REQUIRE(readdy::testing::vec3eq(aParticle2->pos(), readdy::Vec3(0., 0., -1.)));
        }
    }
}

TEST_CASE("Test breaking bonds via diffusion in external potential", "[breakbonds]") {
    GIVEN("A linear polymer with breakable bonds") {
        //readdy::model::Context ctx;
        //readdy::Simulation simulation("CPU", ctx);
        WHEN("an external potential pulls the ends apart") {
            // actions
            THEN("some bond will break because the energy threshold is exceeded") {

            }
        }
    }
}
