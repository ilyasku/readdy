//
// Created by clonker on 07.03.16.
//

#include <readdy/plugin/Kernel.h>
#include <boost/algorithm/string/predicate.hpp>
#include "gtest/gtest.h"

namespace plug = readdy::plugin;

namespace {
    TEST(Kernel, LoadingNonexistingPlugin) {
        plug::Kernel k("foo");
        plug::KernelProvider::getInstance().add(k);
        try{
            plug::KernelProvider::getInstance().get("foo2");
            FAIL() << "Expected NoSuchPluginException!";
        } catch(plug::NoSuchPluginException const &ex) {
            SUCCEED() << "NoSuchPluginException caught.";
        } catch(...) {
            FAIL() << "Expected NoSuchPluginException!";
        }
    }

    TEST(Kernel, LoadingExistingPlugin) {
        plug::Kernel k("bar");
        plug::KernelProvider::getInstance().add(k);
        auto kk_ptr = plug::KernelProvider::getInstance().get("bar");
        EXPECT_STREQ("bar", kk_ptr.get()->getName().c_str());
    }

    TEST(KernelProvider, SanityCheckDefaultDirectory) {
        std::string defaultDirectory = plug::KernelProvider::getInstance().getDefaultKernelDirectory();
        BOOST_LOG_TRIVIAL(debug) << "default directory is " << defaultDirectory;
        SUCCEED();
    }

    TEST(KernelProvider, TestLoadPluginsFromDirectory) {
        // if we're in conda
        const char* env = std::getenv("CONDA_DEFAULT_ENV");
        std::string pluginDir = "lib/readdy_plugins";
        if(env) {
            auto _env = std::string(env);
            if(!boost::algorithm::ends_with(env, "/")) {
                _env = _env.append("/");
            }
            pluginDir = _env.append(pluginDir);
        }
        plug::KernelProvider::getInstance().loadKernelsFromDirectory(pluginDir);
        BOOST_LOG_TRIVIAL(debug) << "current path: " << boost::filesystem::current_path().string();
        std::cout << "refcount == " << plug::KernelProvider::getInstance().get("SingleCPU").use_count() << std::endl;
    }

    TEST(KernelProvider, TestFoo) {
        auto name = plug::KernelProvider::getInstance().get("SingleCPU").get()->getName();
        BOOST_LOG_TRIVIAL(debug) << "foo name: " << name;
    }
}
