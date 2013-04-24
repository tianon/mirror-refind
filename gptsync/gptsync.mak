#
# gptsync/gptsync.mak
# Build control file for the gptsync tool
# 

#
# Include sdk.env environment
#

!include $(SDK_INSTALL_DIR)\build\$(SDK_BUILD_ENV)\sdk.env

#
# Set the base output name and entry point
#

BASE_NAME         = gptsync
IMAGE_ENTRY_POINT = efi_main

#
# Globals needed by master.mak
#

TARGET_APP = $(BASE_NAME)
SOURCE_DIR = $(SDK_INSTALL_DIR)\refit\$(BASE_NAME)
BUILD_DIR  = $(SDK_BUILD_DIR)\refit\$(BASE_NAME)

#
# Include paths
#

!include $(SDK_INSTALL_DIR)\include\$(EFI_INC_DIR)\makefile.hdr
INC = -I $(SDK_INSTALL_DIR)\include\$(EFI_INC_DIR) \
      -I $(SDK_INSTALL_DIR)\include\$(EFI_INC_DIR)\$(PROCESSOR) \
      -I $(SDK_INSTALL_DIR)\refit\include $(INC)

#
# Libraries
#

LIBS = $(LIBS) $(SDK_BUILD_DIR)\lib\libefi\libefi.lib

#
# Default target
#

all : dirs $(LIBS) $(OBJECTS)
	@echo Copying $(BASE_NAME).efi to current directory
	@copy $(SDK_BIN_DIR)\$(BASE_NAME).efi $(BASE_NAME)_$(SDK_BUILD_ENV).efi

#
# Program object files
#

OBJECTS = $(OBJECTS) \
    $(BUILD_DIR)\$(BASE_NAME).obj \
    $(BUILD_DIR)\lib.obj \
    $(BUILD_DIR)\os_efi.obj \

#
# Source file dependencies
#

$(BUILD_DIR)\$(BASE_NAME).obj : $(*B).c $(INC_DEPS)
$(BUILD_DIR)\lib.obj	      : $(*B).c $(INC_DEPS)
$(BUILD_DIR)\os_efi.obj	      : $(*B).c $(INC_DEPS)

#
# Handoff to master.mak
#

!include $(SDK_INSTALL_DIR)\build\master.mak
