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


//
// Created by mho on 10/08/16.
//

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
#include <pybind11/stl_bind.h>
#include <pybind11/stl.h>

#include <readdy/model/Vec3.h>
#include <readdy/io/BloscFilter.h>
#include "SpdlogPythonSink.h"

namespace py = pybind11;
using rvp = py::return_value_policy;

/**
 * Notice: Exporting classes here that are to be shared between prototyping and api module require the base
 * class to use be exported (preferably by the READDY_READDY_API macro defined in common/macros.h).
 */

void exportIO(py::module &);

void exportUtils(py::module& m);

void exportCommon(py::module& common) {
    using namespace pybind11::literals;
    common.def("set_logging_level", [](const std::string &level, bool python_console_out) -> void {
        spdlog::drop("console");
        spdlog::set_sync_mode();
        std::vector<spdlog::sink_ptr> sinks;
        auto sink_stdout = std::make_shared<spdlog::sinks::ansicolor_sink>(spdlog::sinks::stdout_sink_mt::instance());
        sinks.push_back(sink_stdout);
        if(python_console_out) {
            auto sink_pysink = std::make_shared<readdy::rpy::pysink>();
            sinks.push_back(sink_pysink);
        }
        auto logger =  spdlog::create("console", std::begin(sinks), std::end(sinks));
        logger->set_pattern("[          ] [%Y-%m-%d %H:%M:%S] [%t] [%l] %v");
        logger->set_level([&level] {
            if (level == "trace") {
                return spdlog::level::trace;
            }
            if (level == "debug") {
                return spdlog::level::debug;
            }
            if (level == "info") {
                return spdlog::level::info;
            }
            if (level == "warn") {
                return spdlog::level::warn;
            }
            if (level == "err" || level == "error") {
                return spdlog::level::err;
            }
            if (level == "critical") {
                return spdlog::level::critical;
            }
            if (level == "off") {
                return spdlog::level::off;
            }
            readdy::log::warn("Did not select a valid logging level, setting to debug!");
            return spdlog::level::debug;
        }());

        if(python_console_out) {
            // update python loggers level
            py::gil_scoped_acquire gil;
            auto logging_module = pybind11::module::import("logging");
            auto args = py::dict("format"_a="%(message)s");
            switch(logger->level()) {
                case spdlog::level::trace:{
                    /* fall through */
                }
                case spdlog::level::debug: {
                    args["level"] = "DEBUG";
                    break;
                }
                case spdlog::level::info: {
                    args["level"] = "INFO";
                    break;
                }
                case spdlog::level::warn: {
                    args["level"] = "WARNING";
                    break;
                }
                case spdlog::level::err: {
                    args["level"] = "ERROR";
                    break;
                }
                case spdlog::level::critical: {
                    /* fall through */
                }
                case spdlog::level::off: {
                    args["level"] = "CRITICAL";
                    break;
                }
            }
            logging_module.attr("basicConfig")(**args);
        }
    }, "Function that sets the logging level. Possible arguments: \"trace\", \"debug\", \"info\", \"warn\", "
                       "\"err\", \"error\", \"critical\", \"off\".", "level"_a, "python_console_out"_a = true);
    common.def("register_blosc_hdf5_plugin", []() -> void {
        readdy::io::BloscFilter filter;
        filter.registerFilter();
    });
    {
        py::module io = common.def_submodule("io", "ReaDDy IO module");
        exportIO(io);
    }
    {
        py::module util = common.def_submodule("util", "ReaDDy util module");
        exportUtils(util);
    }
    {
        py::module perf = common.def_submodule("perf", "ReaDDy performance module");
        py::class_<readdy::util::PerformanceNode>(perf, "PerformanceNode")
                .def("__getitem__", [](const readdy::util::PerformanceNode &self, const std::string &label) -> const readdy::util::PerformanceNode& {
                    return self.child(label);
                }, rvp::reference)
                .def("__len__", [](const readdy::util::PerformanceNode &self) -> std::size_t {
                    return self.n_children();
                })
                .def("__repr__", [](const readdy::util::PerformanceNode &self) -> std::string {
                    return self.describe();
                })
                .def("clear", &readdy::util::PerformanceNode::clear)
                .def("time", [](const readdy::util::PerformanceNode &self) -> readdy::util::PerformanceData::time {
                    return self.data().cumulativeTime;
                })
                .def("count", [](const readdy::util::PerformanceNode &self) -> std::size_t {
                    return self.data().count;
                });
    }

    py::class_<readdy::model::Vec3>(common, "Vec")
            .def(py::init<readdy::scalar, readdy::scalar, readdy::scalar>())
            .def(py::self + py::self)
            .def(py::self - py::self)
            .def(readdy::scalar() * py::self)
            .def(py::self / readdy::scalar())
            .def(py::self += py::self)
            .def(py::self *= readdy::scalar())
            .def(py::self == py::self)
            .def(py::self != py::self)
            .def(py::self * py::self)
            .def("__repr__", [](const readdy::model::Vec3 &self) {
                std::ostringstream stream;
                stream << self;
                return stream.str();
            })
            .def("__getitem__", [](const readdy::model::Vec3 &self, unsigned int i) {
                return self[i];
            });


}
