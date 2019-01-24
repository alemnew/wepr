all: x86 mips

x86: FORCE
	-mkdir x86
	$(GCC) -shared -fPIC -o x86/libleonetools.so *.c -lm
	$(GCC) -c buffer.c -o x86/buffer.o
	$(GCC) -c hash_table.c -o x86/hash_table.o
	$(GCC) -c logging.c -o x86/logging.o
	$(GCC) -c allocstrcpy.c -o x86/allocstrcpy.o
	ar rvs x86/libleonetools.a x86/*.o

mips: FORCE
	-mkdir mips
	$(MIPS_GCC) -shared -fPIC -o mips/libleonetools.so *.c -lm
	$(MIPS_GCC) -c buffer.c -o mips/buffer.o
	$(MIPS_GCC) -c hash_table.c -o mips/hash_table.o
	$(MIPS_GCC) -c logging.c -o mips/logging.o
	$(MIPS_GCC) -c allocstrcpy.c -o mips/allocstrcpy.o
	ar rvs mips/libleonetools.a mips/*.o
	
FORCE:

clean: FORCE
	rm -rf mips
	rm -rf x86
