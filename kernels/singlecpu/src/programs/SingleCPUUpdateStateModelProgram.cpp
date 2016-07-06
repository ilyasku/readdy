#include <readdy/kernel/singlecpu/programs/SingleCPUUpdateStateModelProgram.h>

/**
 * << detailed description >>
 *
 * @file SingleCPUUpdateStateModelProgram.cpp
 * @brief << brief description >>
 * @author clonker
 * @date 20.06.16
 */


readdy::kernel::singlecpu::programs::SingleCPUUpdateStateModelProgram::SingleCPUUpdateStateModelProgram(SingleCPUKernel *kernel) : kernel(kernel) {
}


void readdy::kernel::singlecpu::programs::SingleCPUUpdateStateModelProgram::execute() {
    kernel->getKernelStateModel().updateModel(curr_t, updateForces);
}

