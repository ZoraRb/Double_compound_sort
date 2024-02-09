#
# this file was created by a computer. trust it.
#

# compiler tools
XILINX_VITIS ?= /home/zohra/Vitis/2022.1
XILINX_XRT ?= /opt/xilinx/xrt
XILINX_VIVADO ?= /home/zohra/Vivado/2022.1
XILINX_HLS ?= $(XILINX_VITIS)/Vivado_HLS

VPP ?= ${XILINX_VITIS}/bin/v++
RM = rm -f
RMDIR = rm -rf

VITIS_PLATFORM = xilinx_u250_gen3x16_xdma_2_1_202010_1
VITIS_PLATFORM_PATH = $(VITIS_PLATFORM)

TARGET = sw_emu
VPP_OPTS = --target $(TARGET)

#
# Hw kernel files
#


BUILD_SUBDIRS += build
HW_KERNEL_OBJS += build/vadd.xo
SRC_KERNEL_OBJS += build/vadd.xo

ALL_MESSAGE_FILES = $(subst .xo,.o,.mdb,$(SRC_KERNEL_OBJS))

#
# primary build targets
#

.PHONY: all clean
all: $(HW_KERNEL_OBJS)


clean:
	-$(RM) $(SRC_KERNEL_OBJS) $(ALL_MESSAGE_FILES) 
	-$(RMDIR) $(BUILD_SUBDIRS)
	-$(RMDIR) .Xil

.PHONY: incremental
incremental: all


nothing:

#
# Hw Kernels compile;
#

build/vadd.xo: ../src/vadd.cpp vadd-compile.cfg
	-@mkdir -p $(@D)
	-@$(RM) $@
	$(VPP) $(VPP_OPTS) --compile -I"$(<D)" --config vadd-compile.cfg -I/home/zohra/Vitis_Libraries-2022.1/database/L1/include/sw -I/home/zohra/Vitis_Libraries-2022.1/database/L1/include/hw -I/home/zohra/Vitis_Libraries-2022.1/utils -o"$@" "$<"

