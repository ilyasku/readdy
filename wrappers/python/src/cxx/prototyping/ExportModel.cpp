/**
 * << detailed description >>
 *
 * @file Model.cpp
 * @brief << brief description >>
 * @author clonker
 * @date 08.08.16
 */

#include <boost/python.hpp>

#include <readdy/model/Particle.h>
#include <../PyConverters.h>
#include <readdy/kernel/singlecpu/model/SingleCPUNeighborList.h>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <readdy/kernel/singlecpu/SingleCPUKernelStateModel.h>
#include "ModelWrap.h"

namespace bpy = boost::python;
namespace mpl = boost::mpl;
namespace rpy = readdy::py;

using _rdy_uuid_t = boost::uuids::uuid;

using _rdy_ctx_t = readdy::model::KernelContext;
using _rdy_particle_t = readdy::model::Particle;
using _rdy_vec_t = readdy::model::Vec3;

using _rdy_scpu_model_t = readdy::kernel::singlecpu::SingleCPUKernelStateModel;
using _rdy_scpu_model_wrap_t = readdy::py::Model;

using _rdy_scpu_nl_t = readdy::kernel::singlecpu::model::SingleCPUNeighborList;
using _rdy_scpu_nl_box_t = readdy::kernel::singlecpu::model::Box;
using _rdy_scpu_pd_t = readdy::kernel::singlecpu::model::SingleCPUParticleData;

using _rdy_pot_1 = readdy::model::potentials::PotentialOrder1;
using _rdy_pot_2 = readdy::model::potentials::PotentialOrder2;

const std::function<readdy::model::Vec3(readdy::model::Vec3, readdy::model::Vec3)> getShortestDistanceFunWrap(_rdy_ctx_t& self) {
    auto shortestDifference = self.getShortestDifferenceFun();
    return [shortestDifference](readdy::model::Vec3 v1, readdy::model::Vec3 v2) {
        return shortestDifference(v1, v2);
    };
}

const std::function<double(readdy::model::Vec3, readdy::model::Vec3)> getDistSquaredFunWrap(_rdy_ctx_t& self) {
    auto dist = self.getDistSquaredFun();
    return [dist](readdy::model::Vec3 v1, readdy::model::Vec3 v2) {
        return dist(v1, v2);
    };
}

template<std::size_t owner = 1, class bp = bpy::default_call_policies>
using internal_ref = bpy::return_internal_reference<owner, bp>;

void exportModelClasses() {

    bpy::class_<_rdy_particle_t>("Particle", boost::python::init<double, double, double, unsigned int>())
            .add_property("pos", +[](_rdy_particle_t &self) { return self.getPos(); }, &_rdy_particle_t::setPos)
            .add_property("type", &_rdy_particle_t::getType, &_rdy_particle_t::setType)
            .add_property("id", bpy::make_function(&_rdy_particle_t::getId,
                                                   internal_ref<>()))
            .def(bpy::self == bpy::self)
            .def(bpy::self != bpy::self);

    bpy::class_<_rdy_ctx_t, boost::noncopyable>("Context", bpy::no_init)
            .add_property("kbt", &_rdy_ctx_t::getKBT, &_rdy_ctx_t::setKBT)
            .add_property("timestep", &_rdy_ctx_t::getTimeStep, &_rdy_ctx_t::setTimeStep)
            .add_property("box_size",
                          +[](_rdy_ctx_t &self) { return readdy::model::Vec3(self.getBoxSize()); },
                          +[](_rdy_ctx_t &self, readdy::model::Vec3 vec) { self.setBoxSize(vec[0], vec[1], vec[2]); })
            .add_property("periodic_boundary",
                          +[](_rdy_ctx_t &self) { return rpy::toList(self.getPeriodicBoundary()); },
                          +[](_rdy_ctx_t &self, bpy::list args) {
                              self.setPeriodicBoundary(bpy::extract<bool>(args[0]),
                                                       bpy::extract<bool>(args[1]),
                                                       bpy::extract<bool>(args[2]));
                          }
            )
            .def("set_diffusion_constant", &_rdy_ctx_t::setDiffusionConstant)
            .def("get_diffusion_constant", +[](_rdy_ctx_t &self, std::string type) {
                return self.getDiffusionConstant(type);
            })
            .def("get_fix_position_fun", rpy::adapt_function(&_rdy_ctx_t::getFixPositionFun))
            .def("get_shortest_difference_fun", rpy::adapt_function(&getShortestDistanceFunWrap))
            .def("get_dist_squared_fun", rpy::adapt_function(&getDistSquaredFunWrap))
            .def("get_particle_radius", +[](_rdy_ctx_t& self, std::string type) {return self.getParticleRadius(type);})
            .def("set_particle_radius", &_rdy_ctx_t::setParticleRadius)
            .def("register_conversion_reaction", +[](_rdy_ctx_t &self, readdy::model::reactions::Conversion& r) -> const boost::uuids::uuid& {return self.registerReaction(&r);},  internal_ref<>())
            .def("register_enzymatic_reaction", +[](_rdy_ctx_t &self, readdy::model::reactions::Enzymatic& r) -> const boost::uuids::uuid& {return self.registerReaction(&r);},  internal_ref<>())
            .def("register_fission_reaction", +[](_rdy_ctx_t &self, readdy::model::reactions::Fission& r) -> const boost::uuids::uuid& {return self.registerReaction(&r);}, internal_ref<>())
            .def("register_fusion_reaction", +[](_rdy_ctx_t &self, readdy::model::reactions::Fusion& r) -> const boost::uuids::uuid& {return self.registerReaction(&r);}, internal_ref<>())
            .def("register_decay_reaction", +[](_rdy_ctx_t &self, readdy::model::reactions::Decay& r) -> const boost::uuids::uuid& {return self.registerReaction(&r);}, internal_ref<>())
            .def("register_potential_order_1",
                 +[](_rdy_ctx_t& self, _rdy_pot_1& pot, std::string type) -> const _rdy_uuid_t& {
                     return self.registerOrder1Potential(&pot, type);
                 }, internal_ref<>())
            .def("register_potential_order_2",
                 +[](_rdy_ctx_t& self, _rdy_pot_2& p, std::string t1, std::string t2) -> const _rdy_uuid_t& {
                     return self.registerOrder2Potential(&p, t1, t2);
                 }, internal_ref<>())
            .def("get_particle_type_id", &_rdy_ctx_t::getParticleTypeID)
            .def("configure", &_rdy_ctx_t::configure);

    bpy::class_<_rdy_scpu_model_wrap_t, boost::noncopyable>("Model", bpy::init<_rdy_ctx_t *>())
            .def("remove_particle", &_rdy_scpu_model_t::removeParticle, &_rdy_scpu_model_wrap_t::default_removeParticle)
            .def("get_particle_positions", &_rdy_scpu_model_t::getParticlePositions,
                 &_rdy_scpu_model_wrap_t::default_getParticlePositions)
            .def("get_energy", &_rdy_scpu_model_t::getEnergy, &_rdy_scpu_model_wrap_t::default_getEnergy)
            .def("increase_energy", &_rdy_scpu_model_t::increaseEnergy, &_rdy_scpu_model_wrap_t::default_increaseEnergy)
            .def("get_particle_data", &_rdy_scpu_model_t::getParticleData,
                 &_rdy_scpu_model_wrap_t::default_getParticleData,
                 internal_ref<>())
            .def("get_neighbor_list", &_rdy_scpu_model_t::getNeighborList,
                 &_rdy_scpu_model_wrap_t::default_getNeighborList,
                 internal_ref<>())
            .def("get_particles", &_rdy_scpu_model_t::getParticles, &_rdy_scpu_model_wrap_t::default_getParticles);

    bpy::class_<_rdy_scpu_nl_t, boost::noncopyable>("NeighborList", bpy::init<_rdy_ctx_t *>())
            .def("create", &_rdy_scpu_nl_t::create)
            .def("setup_neighboring_boxes", &_rdy_scpu_nl_t::setupNeighboringBoxes)
            .def("setup_boxes", &_rdy_scpu_nl_t::setupBoxes)
            .def("fill_boxes", &_rdy_scpu_nl_t::fillBoxes);
    bpy::class_<_rdy_scpu_nl_box_t>("NeighborListBox", bpy::init<long, long, long, long>())
            .def("add_neighbor", &_rdy_scpu_nl_box_t::addNeighbor)
            .def_readonly("i", &_rdy_scpu_nl_box_t::i)
            .def_readonly("j", &_rdy_scpu_nl_box_t::j)
            .def_readonly("k", &_rdy_scpu_nl_box_t::k)
            .def_readonly("id", &_rdy_scpu_nl_box_t::id)
            .def_readwrite("particle_indices", &_rdy_scpu_nl_box_t::particleIndices)
            .def_readwrite("neighboring_boxes", &_rdy_scpu_nl_box_t::neighbors);

    bpy::class_<_rdy_scpu_pd_t, boost::noncopyable>("ParticleData")
            .def("swap", &_rdy_scpu_pd_t::swap)
            .def("size", &_rdy_scpu_pd_t::size)
            .def("max_size", &_rdy_scpu_pd_t::max_size)
            .def("empty", &_rdy_scpu_pd_t::empty)
            .def("clear", &_rdy_scpu_pd_t::clear)
            .def("add_particle", &_rdy_scpu_pd_t::addParticle)
            .def("add_particles", &_rdy_scpu_pd_t::addParticles)
            .def("remove_particle",
                 +[](_rdy_scpu_pd_t &self, _rdy_particle_t particle) { self.removeParticle(particle); })
            .def("remove_particle", +[](_rdy_scpu_pd_t &self, std::size_t index) { self.removeParticle(index); })
            .def("is_marked_for_deactivation", &_rdy_scpu_pd_t::isMarkedForDeactivation)
            .def("get_deactivated_index", &_rdy_scpu_pd_t::getDeactivatedIndex)
            .def("get_n_deactivated", &_rdy_scpu_pd_t::getNDeactivated)
            .def("mark_for_deactivation", &_rdy_scpu_pd_t::markForDeactivation)
            .def("deactivate_marked", &_rdy_scpu_pd_t::deactivateMarked)
            .add_property("ids", bpy::range(&_rdy_scpu_pd_t::cbegin_ids, &_rdy_scpu_pd_t::cend_ids))
            .add_property("positions", bpy::range(&_rdy_scpu_pd_t::cbegin_positions, &_rdy_scpu_pd_t::cend_positions))
            .add_property("forces", bpy::range(&_rdy_scpu_pd_t::cbegin_forces, &_rdy_scpu_pd_t::cend_forces))
            .add_property("types", bpy::range(&_rdy_scpu_pd_t::cbegin_types, &_rdy_scpu_pd_t::cend_types))
            .def("__getitem__", +[](_rdy_scpu_pd_t &self, const unsigned int i) { return self[i]; });
}