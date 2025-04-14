CMAKE ?= cmake
PREFIX ?= build
BUILD_TYPE ?= ReleaseWithDebug
CMAKEFLAGS ?= -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)
TARGET ?=

native:
	mkdir -p $(PREFIX)
	$(CMAKE) . -B $(PREFIX) $(CMAKEFLAGS)
	$(MAKE) -C $(PREFIX) $(TARGET)

win-amd64:
	mkdir -p $(PREFIX)-$@
	$(CMAKE) . -B $(PREFIX)-$@ $(CMAKEFLAGS) -DCMAKE_TOOLCHAIN_FILE=cmake/x86_64-w64-mingw32-toolchain.cmake
	$(MAKE) -C $(PREFIX)-$@ $(TARGET)

win-i686:
	mkdir -p $(PREFIX)-$@
	$(CMAKE) . -B $(PREFIX)-$@ $(CMAKEFLAGS) -DCMAKE_TOOLCHAIN_FILE=cmake/i686-w64-mingw32-toolchain.cmake
	$(MAKE) -C $(PREFIX)-$@ $(TARGET)

lin-aarch64:
	mkdir -p $(PREFIX)-$@
	$(CMAKE) . -B $(PREFIX)-$@ $(CMAKEFLAGS) -DCMAKE_TOOLCHAIN_FILE=cmake/aarch64-gcc-toolchain.cmake
	$(MAKE) -C $(PREFIX)-$@ $(TARGET)

android-amd64:
	mkdir -p $(PREFIX)-$@
	$(CMAKE) . -B $(PREFIX)-$@ $(CMAKEFLAGS) -DCMAKE_TOOLCHAIN_FILE=cmake/amd64-android-toolchain.cmake
	$(MAKE) -C $(PREFIX)-$@ $(TARGET)

android-arm7a:
	mkdir -p $(PREFIX)-$@
	$(CMAKE) . -B $(PREFIX)-$@ $(CMAKEFLAGS) -DCMAKE_TOOLCHAIN_FILE=cmake/arm7a-android-toolchain.cmake
	$(MAKE) -C $(PREFIX)-$@ $(TARGET)

android-aarch64:
	mkdir -p $(PREFIX)-$@
	$(CMAKE) . -B $(PREFIX)-$@ $(CMAKEFLAGS) -DCMAKE_TOOLCHAIN_FILE=cmake/aarch64-android-toolchain.cmake
	$(MAKE) -C $(PREFIX)-$@ $(TARGET)

all:
	$(MAKE) native
	$(MAKE) win-amd64
	$(MAKE) win-i686
	$(MAKE) lin-aarch64
	$(MAKE) android-aarch64
	$(MAKE) android-arm7a
	$(MAKE) android-amd64

docs:
	mkdir -p $(PREFIX)
	$(CMAKE) . -B $(PREFIX) $(CMAKEFLAGS) -DBUILD_DOCS=ON
	$(MAKE) -C $(PREFIX) docs

install:
	$(MAKE) native
	$(MAKE) -C $(PREFIX) install
