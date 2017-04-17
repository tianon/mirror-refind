# Makefile for rEFInd

# This program is licensed under the terms of the GNU GPL, version 3,
# or (at your option) any later version.
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

LOADER_DIR=refind
FS_DIR=filesystems
LIBEG_DIR=libeg
MOK_DIR=mok
GPTSYNC_DIR=gptsync
EFILIB_DIR=EfiLib
export EDK2BASE=/usr/local/UDK2014/MyWorkSpace
export REFIND_VERSION='L"0.10.6.2"'

# The "all" target builds with the TianoCore library if possible, but falls
# back on the more easily-installed GNU-EFI library if TianoCore isn't
# installed at $(EDK2BASE)
all:
ifneq ($(wildcard $(EDK2BASE)/*),)
	@echo "Found $(EDK2BASE); building with TianoCore"
	+make tiano
else
	@echo "Did not find $(EDK2BASE); building with GNU-EFI"
	+make gnuefi
endif

# The "fs" target, like "all," attempts to build with TianoCore but falls
# back to GNU-EFI.
fs:
ifneq ($(wildcard $(EDK2BASE)/*),)
	@echo "Found $(EDK2BASE); building with TianoCore"
	+make fs_tiano
else
	@echo "Did not find $(EDK2BASE); building with GNU-EFI"
	+make fs_gnuefi
endif

# Likewise for GPTsync....
GPTsync:
ifneq ($(wildcard $(EDK2BASE)/*),)
	@echo "Found $(EDK2BASE); building with TianoCore"
	+make gptsync_tiano
else
	@echo "Did not find $(EDK2BASE); building with GNU-EFI"
	+make gptsync_gnuefi
endif

# Don't build gptsync under TianoCore by default because it errors out when
# using a cross-compiler on an x86-64 system. Because gptsync is pretty
# useless on ARM64, skipping it is no big deal....
tiano:
	+make MAKEWITH=TIANO AR_TARGET=EfiLib -C $(EFILIB_DIR) -f Make.tiano
	+make MAKEWITH=TIANO AR_TARGET=libeg -C $(LIBEG_DIR) -f Make.tiano
	+make MAKEWITH=TIANO AR_TARGET=mok -C $(MOK_DIR) -f Make.tiano
	+make MAKEWITH=TIANO BUILDME=refind DLL_TARGET=refind -C $(LOADER_DIR) -f Make.tiano
ifneq ($(ARCH),aarch64)
	+make MAKEWITH=TIANO -C $(GPTSYNC_DIR) -f Make.tiano
endif
#	+make MAKEWITH=TIANO -C $(FS_DIR)

gnuefi:
	+make MAKEWITH=GNUEFI -C $(LIBEG_DIR)
	+make MAKEWITH=GNUEFI -C $(MOK_DIR)
	+make MAKEWITH=GNUEFI -C $(EFILIB_DIR)
	+make MAKEWITH=GNUEFI -C $(LOADER_DIR)
	+make MAKEWITH=GNUEFI -C $(GPTSYNC_DIR) gnuefi
#	+make MAKEWITH=GNUEFI -C $(FS_DIR) all_gnuefi

fs_tiano:
	+make MAKEWITH=TIANO -C $(FS_DIR)

fs_gnuefi:
	+make MAKEWITH=GNUEFI -C $(FS_DIR) all_gnuefi

gptsync_tiano:
	+make MAKEWITH=TIANO -C $(GPTSYNC_DIR) -f Make.tiano

gptsync_gnuefi:
	+make MAKEWITH=GNUEFI -C $(GPTSYNC_DIR) gnuefi

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
