# Check if required directories are defined by a higher priority Makefile
ifndef OBJDIR
export OBJDIR := ${CURDIR}/obj
$(warning OBJDIR not defined, defaulting to $(OBJDIR).)
endif
ifndef CF4OCL_DIR
export CF4OCL_DIR := $(abspath ${CURDIR}/../../cf4ocl)
endif
ifndef CF4OCL_INCDIR
export CF4OCL_INCDIR := $(abspath $(CF4OCL_DIR)/src/lib)
endif

# Check if the required OpenCL locations are defined by a higher priority 
# Makefile, if not define them.

# Macros required by a specific OpenCL implementation, e.g. -DATI_OS_LINUX
# for AMDAPPSDK on Linux.
CLMACROS +=

# The location of the OpenCL headers. In Debian/Ubuntu you can install 
# the package opencl-headers, so that CLINCLUDES remains empty. Otherwise
# you should specify the location, e.g. -I$$AMDAPPSDKROOT/include for 
# AMDAPPSDK on Linux.
CLINCLUDES +=

# Variables Used by Implicit Rules
CC = gcc
CFLAGS = -Wall -Wextra -ggdb3 -std=c99 `pkg-config --cflags glib-2.0` -I$(CF4OCL_INCDIR) $(CLINCLUDES) $(CLMACROS)

# Phony targets
.PHONY: all clean mkdirs

# Targets and rules
all: mkdirs clo_common.o

clo_common.o: clo_common.c clo_common.h
	$(CC) $(CFLAGS) -c $< -o $(OBJDIR)/$@

mkdirs:
	mkdir -p $(OBJDIR)
	
clean:
	rm -rf $(OBJDIR)
