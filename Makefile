PREFIX ?= build

release:
	mkdir -p $(PREFIX)
	cmake . -B $(PREFIX) -DCMAKE_BUILD_TYPE=ReleaseWithDebug
	make -C $(PREFIX)

debug:
	mkdir -p $(PREFIX)
	cmake . -B $(PREFIX) -DCMAKE_BUILD_TYPE=Debug
	make -C $(PREFIX)
