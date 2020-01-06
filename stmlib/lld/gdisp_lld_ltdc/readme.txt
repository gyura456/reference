To use this driver:

1. Add in your gfxconf.h:
	a) #define GFX_USE_GDISP		TRUE

2. To your makefile add the following lines:
	include $(GFXLIB)/gfx.mk
	include $(STMLIB)/lld/gdisp_lld_ltdc/driver.mk

3. Add a board_STM32LTDC.h to you project directory (or board directory)
	based on one of the templates.
