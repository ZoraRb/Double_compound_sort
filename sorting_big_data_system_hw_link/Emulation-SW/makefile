#
# this file was created by a computer. trust it.
#

# compiler tools
XILINX_VITIS ?= /home/zohra/Vitis/2022.1
XILINX_XRT ?= /opt/xilinx/xrt
XILINX_VIVADO ?= /home/zohra/Vivado/2022.1
XILINX_HLS ?= $(XILINX_VITIS)/Vivado_HLS

VPP_LINKER ?= ${XILINX_VITIS}/bin/v++
RM = rm -f
RMDIR = rm -rf

VITIS_PLATFORM = xilinx_u250_gen3x16_xdma_2_1_202010_1
VITIS_PLATFORM_PATH = $(VITIS_PLATFORM)

VPP_OPTS = --target sw_emu

#
# Hw kernel files
#

BINARY_CONTAINERS += vadd.xclbin

BUILD_SUBDIRS += vadd.build
VADD_OBJS += ../../sorting_big_data_kernels/Emulation-SW/build/vadd.xo

ALL_MESSAGE_FILES = $(subst .xclbin,.mdb,$(BINARY_CONTAINERS))

#
# primary build targets
#

.PHONY: all clean
all: $(BINARY_CONTAINERS)


clean:
	-$(RM) $(BINARY_CONTAINERS) $(ALL_MESSAGE_FILES) 
	-$(RM) *.xclbin.sh *.xclbin.info *.xclbin.link_summary*
	-$(RMDIR) $(BUILD_SUBDIRS)
	-$(RMDIR) .Xil

ifeq ($(MAKECMDGOALS),incremental)
.PHONY: vadd.xclbin
.PHONY: ../../sorting_big_data_kernels/Emulation-SW/build/vadd.xo
endif

.PHONY: incremental
incremental: all


nothing:

#
# binary container: vadd.xclbin
#

vadd.xclbin: $(VADD_OBJS) vadd-link.cfg
	-@echo $(VPP_LINKER) $(VPP_OPTS) --link --config vadd-link.cfg -o"$@" $(VADD_OBJS) > vadd.xclbin.sh
	$(VPP_LINKER) $(VPP_OPTS) --link --config vadd-link.cfg -o"$@" $(VADD_OBJS)

