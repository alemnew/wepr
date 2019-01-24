all: x86 mips

x86: FORCE
	-mkdir x86
	ar rvs x86/libleone.a ../dns-library/x86/*.o ../tools/x86/*.o ../libhurl/*.o
	
	$(GCC) -I../dns-library/  -I../tools/ -I../libhurl/  -Lx86/ *.c -o x86/webperf \
	x86/libleone.a -Wl,-Bstatic -lleone -Wl,-Bdynamic -lm -lcrypto -lssl -pthread
	
mips: FORCE
	-mkdir mips
	ar rvs mips/libleone.a ../dns-library/mips/*.o ../tools/mips/*.o ../libhurl/mips/*.o
	
	$(MIPS_GCC) \
	-I../dns-library/  -I../tools/  -I$(LIBHURL_DIR)/ \
	-I$(OPENWRTDIR)/staging_dir/target-mips_34kc_uClibc-0.9.33.2/usr/include/ \
	-L$(OPENWRTDIR)/staging_dir/target-mips_34kc_uClibc-0.9.33.2/usr/lib \
	-Lmips/ -L$(OPENWRTDIR)/staging_dir/target-mips_34kc_uClibc-0.9.33.2/usr/lib/ \
	*.c -o mips/webperf mips/libleone.a -Wl,-Bstatic -lleone -Wl,-Bdynamic -lm -lcrypto \
	-lssl -pthread
	$(MIPS_STRIP) mips/webperf

	
clean: FORCE
	rm -rf mips
	rm -rf x86
	
	
FORCE:
