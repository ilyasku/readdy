/********************************************************************
 * Copyright © 2016 Computational Molecular Biology Group,          *
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the    *
 * GNU Lesser General Public License for more details.              *
 *                                                                  *
 * You should have received a copy of the GNU Lesser General        *
 * Public License along with this program. If not, see              *
 * <http://www.gnu.org/licenses/>.                                  *
 ********************************************************************/


/**
 * << detailed description >>
 *
 * @file SingleCPUReactions.h
 * @brief << brief description >>
 * @author clonker
 * @date 21.06.16
 */

#ifndef READDY_MAIN_SINGLECPUREACTIONS_H
#define READDY_MAIN_SINGLECPUREACTIONS_H

#include <readdy/model/reactions/Conversion.h>
#include <readdy/model/Particle.h>
#include <readdy/model/reactions/Enzymatic.h>
#include <readdy/model/reactions/Fission.h>
#include <readdy/model/reactions/Fusion.h>
#include <readdy/model/RandomProvider.h>
#include <readdy/common/make_unique.h>

namespace readdy {
namespace kernel {
namespace scpu {
namespace reactions {

class SCPUConversion : public readdy::model::reactions::Conversion {

public:
    SCPUConversion(const std::string &name, unsigned int typeFrom, unsigned int typeTo, const double rate)
            : readdy::model::reactions::Conversion(name, typeFrom, typeTo, rate) {}

    virtual void perform(const readdy::model::Particle &p1_in, const readdy::model::Particle &p2_in,
                         readdy::model::Particle &p1_out,
                         readdy::model::Particle &p2_out, rnd_normal rnd) const override;
};

class SCPUEnzymatic : public readdy::model::reactions::Enzymatic {

public:
    SCPUEnzymatic(const std::string &name, unsigned int catalyst, unsigned int from, unsigned int to,
              const double rate, const double eductDistance)
            : readdy::model::reactions::Enzymatic(name, catalyst, from, to, rate, eductDistance) {}

    virtual void perform(const readdy::model::Particle &p1_in, const readdy::model::Particle &p2_in,
                         readdy::model::Particle &p1_out,
                         readdy::model::Particle &p2_out, rnd_normal rnd) const override;
};

class SCPUFission : public readdy::model::reactions::Fission {

public:
    SCPUFission(const std::string &name, unsigned int from, unsigned int to1, unsigned int to2,
            const double &rate, const double productDistance, const double weight1 = .5,
            const double weight2 = .5)
            : readdy::model::reactions::Fission(name, from, to1, to2, rate, productDistance, weight1, weight2) {}

    virtual void perform(const readdy::model::Particle &p1_in, const readdy::model::Particle &p2_in,
                         readdy::model::Particle &p1_out,
                         readdy::model::Particle &p2_out, rnd_normal rnd) const override;

    SCPUFission(const SCPUFission &rhs)
            : SCPUFission(rhs.name, rhs.educts[0], rhs.products[0], rhs.products[1],
                      rhs.rate, rhs.productDistance, rhs.weight1, rhs.weight2) {}

protected:

};

class SCPUFusion : public readdy::model::reactions::Fusion {

public:
    SCPUFusion(const std::string &name, unsigned int from1, unsigned int from2, unsigned int to,
           const double rate, const double eductDistance, const double weight1 = 0.5,
           const double weight2 = 0.5)
            : readdy::model::reactions::Fusion(name, from1, from2, to, rate, eductDistance, weight1, weight2) {}

    virtual void perform(const readdy::model::Particle &p1_in, const readdy::model::Particle &p2_in,
                         readdy::model::Particle &p1_out,
                         readdy::model::Particle &p2_out, rnd_normal rnd) const override;

};

}
}
}
}


#endif //READDY_MAIN_SINGLECPUREACTIONS_H