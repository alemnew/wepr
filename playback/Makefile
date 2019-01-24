all: x86

x86: FORCE
	-mkdir x86
	ar rvs x86/libleone.a ../dns-library/x86/*.o \
	../tools/x86/*.o \
	../../libhurl/*.o
	#$(WORKSPACE)
	$(GCC) -g -o0 \
	-I../dns-library/  \
	-I../tools/ \
	-I../parsing-server/ \
	-I../../libhurl/ \
	-Lx86/ \
	*.c -o x86/playback \
	x86/libleone.a \
	-Wl,-Bstatic -lleone \
	-Wl,-Bdynamic -lm -ljansson -lcrypto -lssl -pthread
	
clean: FORCE
	rm -rf x86
	
	
FORCE:
