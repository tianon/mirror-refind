# Makefile for rEFInd

# This program is licensed under the terms of the GNU GPL, version 3,
# or (at your option) any later version.
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

include Make.common

LOADER_DIR=refind
FS_DIR=filesystems
LIBEG_DIR=libeg
MOK_DIR=mok
GPTSYNC_DIR=gptsync
EFILIB_DIR=EfiLib
export EDK2BASE=/usr/local/UDK2014/MyWorkSpace
#export EDK2BASE=/home/rodsmith/programming/edk2-UDK2017
-include $(EDK2BASE)/Conf/target.txt
THISDIR=$(shell pwd)
EDK2_BUILDLOC=$(EDK2BASE)/Build/Refind/$(TARGET)_$(TOOL_CHAIN_TAG)/$(UC_ARCH)
EDK2_FILES=REFIND GPTSYNC BTRFS EXT4 EXT2 HFS ISO9660 NTFS REISERFS
EDK2_PROGS=$(EDK2_FILES:=.efi)

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

# Don't build gptsync under TianoCore/AARCH64 by default because it errors out
# when using a cross-compiler on an x86-64 system. Because gptsync is pretty
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

# Build process for TianoCore using TianoCore-standard build process rather
# than my own custom Makefiles (except this top-level one)
edk2: $(EDK2_BUILDLOC)/$(EDK2_PROGS)

$(EDK2_BUILDLOC)/$(EDK2_PROGS): $(EDK2BASE)/RefindPkg
	$(info $$THISDIR is [${THISDIR}])
	cd $(EDK2BASE) && \
	. ./edksetup.sh BaseTools && \
	build -a $(UC_ARCH) -p RefindPkg/RefindPkg.dsc
	cp $(EDK2_BUILDLOC)/REFIND.efi ./refind/refind_$(FILENAME_CODE).efi
	cp $(EDK2_BUILDLOC)/GPTSYNC.efi ./gptsync/refind_$(FILENAME_CODE).efi
	cp $(EDK2_BUILDLOC)/EXT2.efi ./drivers_$(FILENAME_CODE)/ext2_$(FILENAME_CODE).efi
	cp $(EDK2_BUILDLOC)/EXT4.efi ./drivers_$(FILENAME_CODE)/ext4_$(FILENAME_CODE).efi
	cp $(EDK2_BUILDLOC)/BTRFS.efi ./drivers_$(FILENAME_CODE)/btrfs_$(FILENAME_CODE).efi
	cp $(EDK2_BUILDLOC)/REISERFS.efi ./drivers_$(FILENAME_CODE)/reiserfs_$(FILENAME_CODE).efi
	cp $(EDK2_BUILDLOC)/NTFS.efi ./drivers_$(FILENAME_CODE)/ntfs_$(FILENAME_CODE).efi
	cp $(EDK2_BUILDLOC)/HFS.efi ./drivers_$(FILENAME_CODE)/hfs_$(FILENAME_CODE).efi
	cp $(EDK2_BUILDLOC)/ISO9660.efi ./drivers_$(FILENAME_CODE)/iso9660_$(FILENAME_CODE).efi

$(EDK2BASE)/RefindPkg:
	ln -s $(THISDIR) $(EDK2BASE)/RefindPkg

# Filesystem and gptsync build rules....

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
	rm -rf $(EDK2BASE)/Build/Refind

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
