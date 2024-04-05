RELEASE = mupen64plus
SUBDIRS = \
    mupen64plus-core \
    mupen64plus-rsp-hle \
    mupen64plus-audio-sdl \
    mupen64plus-input-sdl \
    mupen64plus-ui-console \
    mupen64plus-video-glide64mk2

export OSD=0
export HIRES=0
export VULKAN=0
export USE_GLES=1
export USE_FRAMESKIPPER=1

MOD = mmiyoo
ifeq ($(MOD),mmiyoo)
    export PATH=/opt/mmiyoo/bin:$(shell printenv PATH)
    export HOST_CPU=arm-linux
    export CROSS_COMPILE=arm-linux-gnueabihf-
    export NEON=1
    export PKG_CONFIG=/opt/mmiyoo/bin/pkg-config
    export SDL_CONFIG=/opt/mmiyoo/arm-buildroot-linux-gnueabihf/sysroot/usr/bin/sdl2-config
    $(shell cp -a assets/mmiyoo/* $(RELEASE))
endif

define FOREACH
    for DIR in $(SUBDIRS); do \
        make -C $$DIR/projects/unix $(1); \
    done
endef

.PHONY: all
all:
	$(call FOREACH,all)
	cp mupen64plus-ui-console/projects/unix/mupen64plus $(RELEASE)
	cp mupen64plus-core/projects/unix/libmupen64plus.so.2 $(RELEASE)
	cp mupen64plus-rsp-hle/projects/unix/mupen64plus-rsp-hle.so $(RELEASE)
	cp mupen64plus-audio-sdl/projects/unix/mupen64plus-audio-sdl.so $(RELEASE)
	cp mupen64plus-input-sdl/projects/unix/mupen64plus-input-sdl.so $(RELEASE)
	cp mupen64plus-video-glide64mk2/projects/unix/mupen64plus-video-glide64mk2.so $(RELEASE)
	cp -a mupen64plus-core/data/* $(RELEASE)
	cp -a mupen64plus-input-sdl/data/* $(RELEASE)
	cp -a mupen64plus-ui-console/data/* $(RELEASE)
	cp -a mupen64plus-video-glide64mk2/data/* $(RELEASE)

.PHONY: clean
clean:
	rm -rf $(RELEASE)
	$(call FOREACH,clean)
	mkdir -p $(RELEASE)/lib
