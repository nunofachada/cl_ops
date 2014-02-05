# Check if required directories are defined by a higher priority Makefile
ifndef OBJDIR
export OBJDIR := ${CURDIR}/obj
endif
ifndef BUILDDIR
export BUILDDIR := ${CURDIR}/bin
endif
ifndef CF4OCL_DIR
export CF4OCL_DIR := $(abspath ${CURDIR}/cf4ocl)
endif
ifndef CF4OCL_INCDIR
export CF4OCL_INCDIR := $(abspath $(CF4OCL_DIR)/src/lib)
endif
ifndef CF4OCL_OBJDIR
export CF4OCL_OBJDIR := $(OBJDIR)
endif

# Call "make" on the following folders
SUBDIRS = src $(CF4OCL_DIR)

# Phony targets
.PHONY: all $(SUBDIRS) clean mkdirs getutils

# Targets and rules
all: $(SUBDIRS)

$(SUBDIRS): mkdirs
	$(MAKE) -C $@
     
$(CF4OCL_DIR): getutils

src: $(CF4OCL_DIR)

getutils:
	test -d $(CF4OCL_DIR) || git clone https://github.com/FakenMC/cf4ocl.git

mkdirs:
	mkdir -p $(BUILDDIR)
	mkdir -p $(OBJDIR)
	
clean:
	rm -rf $(OBJDIR) $(BUILDDIR)


     
