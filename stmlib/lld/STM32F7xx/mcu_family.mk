# Stmlib subsytem files.
ifeq ($(USE_SMART_BUILD),yes)
STMLIBCONF := $(strip $(shell cat stmlib_conf.h | egrep -e "\#define"))
STMLIBSRC += $(STMLIB)/lld/$(MCU_FAMILY)/stmlib_lld.c
STMLIBINC += $(STMLIB)/lld/$(MCU_FAMILY)

ifneq ($(findstring USE_SDRAM_DRIVER TRUE,$(STMLIBCONF)),)
STMLIBSRC += $(STMLIB)/lld/sdram/sdram_lld.c \
	$(STMLIB)/lld/fmc/fmc_lld.c

STMLIBINC += $(STMLIB)/lld/sdram \
	  $(STMLIB)/lld/fmc
endif
else
STMLIBSRC += $(STMLIB)/lld/$(MCU_FAMILY)/stmlib_lld.c \
	$(STMLIB)/lld/sdram/sdram_lld.c \
	$(STMLIB)/lld/fmc/fmc_lld.c

STMLIBINC += $(STMLIB)/lld/$(MCU_FAMILY) \
	  $(STMLIB)/lld/sdram \
	  $(STMLIB)/lld/fmc
endif