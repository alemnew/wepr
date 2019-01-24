#!/bin/bash
if [ ! -d "$LIBHURL_DIR" ]; then
    echo "libhurl directory NOT found."
    exit 1
fi

cd $LIBHURL_DIR
mkdir mips 2>/dev/null
$MIPS_GCC -I $OPENWRTDIR/staging_dir/target-mips_34kc_uClibc-0.9.33.2/usr/include \
	-L$OPENWRTDIR/staging_dir/target-mips_34kc_uClibc-0.9.33.2/usr/lib \
	-lm -Bdynamic -lcrypto -lssl -c hurl_core.c -o mips/hurl_core.o
$MIPS_GCC -I $OPENWRTDIR/staging_dir/target-mips_34kc_uClibc-0.9.33.2/usr/include \
	-L$(OPENWRTDIR)/staging_dir/target-mips_34kc_uClibc-0.9.33.2/usr/lib  -lm -Bdynamic -lcrypto -lssl -c hurl_parse.c -o mips/hurl_parse.o
ar rvs mips/libhurl.a mips/*.o
