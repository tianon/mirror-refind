#
# refind/Makefile
# Build control file for the rEFInd boot menu
#

SRCDIR = .

VPATH = $(SRCDIR)

ARCH            = $(shell uname -m | sed s,i[3456789]86,ia32,)

TARGET	= refind.efi

ifeq ($(ARCH),ia32)
  LIBEG = build32
  TARGET = refind_ia32.efi
endif

ifeq ($(ARCH),x86_64)
  LIBEG = build64
  TARGET = refind_x64.efi
endif

ifeq ($(ARCH),aarch64)
  LIBEG = build
  TARGET = refind_aa64.efi
endif

LOCAL_CPPFLAGS  = -I$(SRCDIR) -I$(SRCDIR)/../include -I$(SRCDIR)/../libeg -I$(SRCDIR)/../mok
LOCAL_LDFLAGS   = -L$(SRCDIR)/../libeg/ -L$(SRCDIR)/../mok/ -L$(SRCDIR)/../EfiLib/
LOCAL_LIBS      = -leg -lmok -lEfiLib

OBJS            = main.o mystrings.o apple.o line_edit.o config.o menu.o screen.o icns.o gpt.o crc32.o lib.o driver_support.o legacy.o

all: $(TARGET)

include $(SRCDIR)/../Make.common

$(SHLIB_TARGET): $(OBJS)
	$(LD) $(LOCAL_LDFLAGS) $(LDFLAGS) $(OBJS) -o $@ $(LOCAL_LIBS) $(LIBS)

$(TARGET): $(SHLIB_TARGET)
	$(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel \
	           -j .rela -j .rel.* -j .rela.* -j .rel* -j .rela* \
		   -j .reloc $(FORMAT) $< $@
	chmod a-x $(TARGET)

# EOF
# DO NOT DELETE
