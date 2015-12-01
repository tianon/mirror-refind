# Makefile for rEFInd

# This program is licensed under the terms of the GNU GPL, version 3,
# or (at your option) any later version.
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

CXXFLAGS=-O2 -fpic -D_REENTRANT -D_GNU_SOURCE -Wall -g
NAMES=refind
SRCS=$(NAMES:=.c)
OBJS=$(NAMES:=.o)
HEADERS=$(NAMES:=.h)
LOADER_DIR=refind
FS_DIR=filesystems
LIBEG_DIR=libeg
MOK_DIR=mok
GPTSYNC_DIR=gptsync
EFILIB_DIR=EfiLib

export EDK2BASE        = /usr/local/UDK2014/MyWorkSpace
export GENFW           = $(EDK2BASE)/BaseTools/Source/C/bin/GenFw
export prefix          = /usr/bin/
ifeq ($(ARCH),aarch64)
  export CC            = $(prefix)aarch64-linux-gnu-gcc
  export AS            = $(prefix)aarch64-linux-gnu-as
  export LD            = $(prefix)aarch64-linux-gnu-ld
  export AR            = $(prefix)aarch64-linux-gnu-ar
  export RANLIB        = $(prefix)aarch64-linux-gnu-ranlib
  export OBJCOPY       = $(prefix)aarch64-linux-gnu-objcopy
else
  export CC            = $(prefix)gcc
  export AS            = $(prefix)as
  export LD            = $(prefix)ld
  export AR            = $(prefix)ar
  export RANLIB        = $(prefix)ranlib
  export OBJCOPY       = $(prefix)objcopy
endif

# Build rEFInd, including libeg
all:	tiano

gnuefi:
	+make -C $(LIBEG_DIR)
	+make -C $(MOK_DIR)
	+make -C $(EFILIB_DIR)
	+make -C $(LOADER_DIR)
	+make -C $(GPTSYNC_DIR) gnuefi
#	+make -C $(FS_DIR) all_gnuefi

fs:
	+make -C $(FS_DIR)

fs_gnuefi:
	+make -C $(FS_DIR) all_gnuefi

tiano:
	+make AR_TARGET=EfiLib -C $(EFILIB_DIR) -f Make.tiano
	+make AR_TARGET=libeg -C $(LIBEG_DIR) -f Make.tiano
	+make AR_TARGET=mok -C $(MOK_DIR) -f Make.tiano
	+make BUILDME=refind DLL_TARGET=refind -C $(LOADER_DIR) -f Make.tiano
ifneq ($(ARCH),aarch64)
	+make -C $(GPTSYNC_DIR) -f Make.tiano
endif
#	+make -C $(FS_DIR)

gptsync:
	+make -C $(GPTSYNC_DIR) -f Make.tiano

gptsync_gnuefi:
	+make -C $(GPTSYNC_DIR) gnuefi

clean:
	make -C $(LIBEG_DIR) clean
	make -C $(MOK_DIR) clean
	make -C $(LOADER_DIR) clean
	make -C $(EFILIB_DIR) clean
	make -C $(FS_DIR) clean
	make -C $(GPTSYNC_DIR) clean
	rm -f include/*~

# NOTE TO DISTRIBUTION MAINTAINERS:
# The "install" target installs the program directly to the ESP
# and it modifies the *CURRENT COMPUTER's* NVRAM. Thus, you should
# *NOT* use this target as part of the build process for your
# binary packages (RPMs, Debian packages, etc.). (Gentoo could
# use it in an ebuild, though....) You COULD, however, copy the
# files to a directory somewhere (/usr/share/refind or whatever)
# and then call refind-install as part of the binary package
# installation process.

install:
	./refind-install

# DO NOT DELETE
