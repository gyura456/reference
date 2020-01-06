# FATFS files.
STMLIBSRC += $(CHIBIOS)/os/various/fatfs_bindings/fatfs_diskio.c \
           $(CHIBIOS)/os/various/fatfs_bindings/fatfs_syscall.c \
           $(STMLIB)/Extensions/fatfs/src/ff.c \
           $(STMLIB)/Extensions/fatfs/src/option/unicode.c

STMLIBINC += $(STMLIB)/Extensions/fatfs/src
