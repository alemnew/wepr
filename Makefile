export OPENWRTDIR=$(HOME)/openwrt
#export STAGING_DIR=$(OPENWRTDIR)/trunk/staging_dir/
export STAGING_DIR=$(OPENWRTDIR)/staging_dir
export MIPS_GCC=$(STAGING_DIR)/toolchain-mips_34kc_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mips-openwrt-linux-uclibc-gcc -std=gnu99 -g0 -Os
export MIPS_STRIP=$(STAGING_DIR)/toolchain-mips_34kc_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mips-openwrt-linux-uclibc-strip
export GCC=/usr/bin/gcc -std=gnu99 -pedantic -Wall
#export GCC=/usr/bin/clang -std=gnu99 -pedantic -Wall -Weverything -Wpadded
#export LIBHURL_DIR=$(HOME)/libhurl
export LIBHURL_DIR=libhurl

all: webperf FORCE

x86: tools-x86 dns-library-x86  libhurl-x86 webperf-x86  playback
#save-body

mips: tools-mips dns-library-mips libhurl-mips webperf-mips

tools-mips: FORCE
	make -C tools mips
	
dns-library-mips:  tools FORCE
	make -C dns-library mips

tools-x86:  FORCE
	make -C tools x86
	
dns-library-x86: tools-x86 FORCE
	make -C dns-library x86
save-body: FORCE
	make -C save-body x86

libhurl: libhurl-x86 libhurl-mips

webperf-x86: dns-library-x86 libhurl-x86 FORCE
	make -C webperf x86

webperf-mips:  dns-library-mips libhurl-mips FORCE
	make -C webperf mips

webperf: webperf-x86 webperf-mips	

playback: tools-x86 libhurl-x86 dns-library-x86
	make -C playback

libhurl-x86: FORCE
	make -C $(LIBHURL_DIR) release-static

libhurl-mips: FORCE
	./libhurl-mips-build.sh
	
clean: FORCE
	make -C tools clean
	make -C dns-library clean
	make -C $(LIBHURL_DIR) clean
	make -C webperf clean
	-rm -rf $(LIBHURL_DIR)/mips
	make -C playback clean

FORCE:
