# Stmlib subsytem files.
ifeq ($(USE_SMART_BUILD),yes)
STMLIBCONF := $(strip $(shell cat stmlib_conf.h | egrep -e "\#define"))
STMLIBSRC += $(STMLIB)/src/stmlib.c
ifneq ($(findstring USE_SDRAM_DRIVER TRUE,$(STMLIBCONF)),)
STMLIBSRC += $(STMLIB)/src/sdram.c
endif
else
STMLIBSRC += $(STMLIB)/src/stmlib.c 
endif
STMLIBINC += $(STMLIB)/include

include $(STMLIB)/lld/$(MCU_FAMILY)/mcu_family.mk