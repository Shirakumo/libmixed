CMAKE ?= cmake
PREFIX ?= build
BUILD_TYPE ?= ReleaseWithDebug
CMAKEFLAGS ?= -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

native:
	mkdir -p $(PREFIX)
	$(CMAKE) . -B $(PREFIX) $(CMAKEFLAGS)
	$(MAKE) -C $(PREFIX)

win32-amd64:
	mkdir -p $(PREFIX)-$@
	$(CMAKE) . -B $(PREFIX)-$@ $(CMAKEFLAGS) -DCMAKE_TOOLCHAIN_FILE=cmake/x86_64-w64-mingw32-toolchain.cmake
	$(MAKE) -C $(PREFIX)-$@

win32-i686:
	mkdir -p $(PREFIX)-$@
	$(CMAKE) . -B $(PREFIX)-$@ $(CMAKEFLAGS) -DCMAKE_TOOLCHAIN_FILE=cmake/i686-w64-mingw32-toolchain.cmake
	$(MAKE) -C $(PREFIX)-$@

linux-aarch64:
	mkdir -p $(PREFIX)-$@
	$(CMAKE) . -B $(PREFIX)-$@ $(CMAKEFLAGS) -DCMAKE_TOOLCHAIN_FILE=cmake/aarch64-gcc-toolchain.cmake
	$(MAKE) -C $(PREFIX)-$@

android-aarch64:
	mkdir -p $(PREFIX)-$@
	$(CMAKE) . -B $(PREFIX)-$@ $(CMAKEFLAGS) -DCMAKE_TOOLCHAIN_FILE=cmake/aarch64-android-toolchain.cmake
	$(MAKE) -C $(PREFIX)-$@

all:
	$(MAKE) native
	$(MAKE) win32-amd64
	$(MAKE) win32-i686
	$(MAKE) linux-aarch64
	$(MAKE) android-aarch64

docs:
	mkdir -p $(PREFIX)
	$(CMAKE) . -B $(PREFIX) $(CMAKEFLAGS) -DBUILD_DOCS=ON
	$(MAKE) -C $(PREFIX) docs

install:
	$(MAKE) native
	$(MAKE) -C $(PREFIX) install
