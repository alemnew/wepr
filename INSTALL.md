
Install
---
**Dependencies**
- OpenSSL
- libhurl


**Compiling for x86**  
1.  Download libhurl from [Github](https://github.com/libhurl/libhurl)  
2. Change the path of libhurl in the makefile if necessary.  
  ``export LIBHURL_DIR=$(HOME)/libhurl.git``  
3.  Run ``make x86``  

The compiled binary is located in ``leone-webperf/x86``.
The libraries not available on the SamKnows probes are statically linked such that a single executable is created.

**Compiling for MIPS (SamKnows probes)**  
1. Install OpenWrt tool-chain as described on [Leone wiki](https://ssl.leone-project.eu/mediawiki/index.php?title=Developing_With_The_SamKnows_Probes).  
1. Update paths to OpenWrt tool-chain in makefile.  
 `` export OPENWRTDIR=$(HOME)/openwrt ``  
``export STAGING_DIR=$(OPENWRTDIR)/trunk/staging_dir/``  
c) Run ``make mips``  

The compiled binary is located in ``leone-webperf/mips``
