##############################################################################
# MCU settings
#
# See in ChibiOS source dir: /os/hal/ports/STM32/$(MCU_FAMILY) -dir name
MCU_FAMILY = STM32F7xx
# Like m0, m4, m7
CORTEX_TYPE = m7
# See in ChibiOS source dir: /os/common/ports/ARMCMx/compillers/mk/$(PORTS) -file name
PORT = port_v7m.mk
# See in ChibiOS source dir: /os/hal/boards/$(BORAD) - dir name
BOARD = ST_STM32F746G_DISCOVERY
# See in ChibiOS source dir: /os/common/startup/ARMCMx/compilers/GCC/mk/$(STARTUP_FILE) - file name
STARTUP_FILE = startup_stm32f7xx.mk
# See in ChibiOS source dir: /os/common/startup/ARMCMx/compilers/GCC/ld/$(LINKER_FILE) - file name
LINKER_FILE = STM32F746xG.ld
#
# MCU settings
##############################################################################


##############################################################################
# Architecture or project specific options
#

# Stack size to be allocated to the Cortex-M process stack. This stack is
# the stack used by the main() thread.
USE_PROCESS_STACKSIZE = 0x400
# Stack size to the allocated to the Cortex-M main/exceptions stack. This
# stack is used for processing interrupts and exceptions.
USE_EXCEPTIONS_STACKSIZE = 0x400

# Enables the use of FPU (no, softfp, hard).
USE_FPU = hard
#
# Architecture or project specific options
##############################################################################

##############################################################################
# Project, sources and paths
#

# Define project name here
PROJECT = beeswax_sterilizer
PROJECTINC = ./include
PROJECTSRC=$(patsubst %.c,%.o,$(wildcard src/*.c))

# Imported source files and paths
CHIBIOS = ../ext_sources/chibios
# Stmlib source path
STMLIB = ../stmlib
# Stmlib options (Like yes or no)
USE_STMLIB = yes
USE_INNER_BUFFER = yes
USE_FAT_FS = yes
USE_NUMKEYS = yes
#uGFX options
#Like yes or no
USE_UGFX = yes
#uGFX source path
GFXLIB = ../ext_sources/ugfx
include $(GFXLIB)/gfx.mk
#ugfx driver includes
include $(STMLIB)/lld/gdisp_lld_ltdc/driver.mk
include $(GFXLIB)/drivers/ginput/touch/FT5336/driver.mk
#ugfx board includes
include $(STMLIB)/ugfx_boardfiles/stm32f746_disc/board.mk
#Devices includes
include $(STMLIB)/devices/displays/rk043fn48h/display.mk
include $(STMLIB)/devices/sdrams/mt48lc4m32b2/sdramdevice.mk
include $(STMLIB)/devices/sensors/temperature/adt7410/sensor.mk

#
# Project, sources and paths
##############################################################################


##############################################################################
# Start of user section
#

# List all user C define here, like -D_DEBUG=1
UDEFS =

# Define ASM defines here
UADEFS =

# List all user directories here
UINCDIR =

# List the user directory to look for the libraries here
ULIBDIR =

# List all user libraries here
ULIBS =

#
# End of user defines
##############################################################################
include settings.mk

help:global_help
	@echo "	createtask"
	@echo "		-create a new task for app"
	@echo ""
.PHONY: help

include targets.mk
include help.mk
