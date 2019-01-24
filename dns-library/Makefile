all: x86 mips

x86: FORCE
	-mkdir x86
	$(GCC)  -I"../tools" -L"../tools/x86/" -shared -fPIC -o x86/libleonedns.so *.c -lm -lleonetools
	
	$(GCC) -I../tools/ -c dns_cache.c -o x86/dns_cache.o
	$(GCC) -I../tools/ -c dns_core.c -o x86/dns_core.o
	$(GCC) -I../tools/ -c dns_json.c -o x86/dns_json.o
	$(GCC) -I../tools/ -c dns_message.c -o x86/dns_message.o
	$(GCC) -I../tools/ -c dns_parser.c -o x86/dns_parser.o
	$(GCC) -I../tools/ -c dns_support.c -o x86/dns_support.o
	ar rvs x86/libleonedns.a x86/*.o

mips: FORCE
	-mkdir mips
	$(MIPS_GCC) \
	-I../dns-library/  \
	-I../tools/ \
	-I$(OPENWRTDIR)/staging_dir/target-mips_34kc_uClibc-0.9.33.2/include/ \
	-L$(OPENWRTDIR)/staging_dir/target-mips_34kc_uClibc-0.9.33.2/usr/lib \
	-lm -Bdynamic -lcrypto -lssl  -pthread -shared -fPIC ../tools/mips/libleonetools.so \
	*.c -o mips/libleonedns.so
	
	$(MIPS_GCC) -I../tools/ -c dns_cache.c -o mips/dns_cache.o
	$(MIPS_GCC) -I../tools/ -c dns_core.c -o mips/dns_core.o
	$(MIPS_GCC) -I../tools/ -c dns_json.c -o mips/dns_json.o
	$(MIPS_GCC) -I../tools/ -c dns_message.c -o mips/dns_message.o
	$(MIPS_GCC) -I../tools/ -c dns_parser.c -o mips/dns_parser.o
	$(MIPS_GCC) -I../tools/ -c dns_support.c -o mips/dns_support.o
	ar rvs mips/libleonedns.a mips/*.o

FORCE:

clean: FORCE
	rm -rf mips
	rm -rf x86
