hurl
---
hURL is a multi-threaded HTTP client which supports persistent connections and request pipelining.

leone-dns-library
---
Library for DNS resolution

leone-playback
---
DNS and HTTP server designed to emulate performance recorded by the Webperf test.

leone-render
---
Rendering performance tests.

leone-tools
---
Library containing shared functions.

leone-webperf
---
The actual Webperf test.
Examples of test configuration files are located under [leone-webperf/testing/conf]

Check 'webperf.conf' for list of configuration options and their meaning.

parsing-server
---
Parsing server that turns HTML files into a list of files that must be downloaded in order to render a webpage.

sk-deploy
---
Scripts and binaries for deployment of Webperf test on SamKnows infrastructure

webperf-paco
---
Scripts and binaries for deployment of Webperf test on Paco's testbed.


How to build
---
Before compiling for MIPS, change OpenWRT build-root paths in main [Makefile].

```sh
    make x86
    make mips
```

The binaries are located in leone-webperf/x86 and leone-webperf/mips



 [leone-webperf/testing/conf]:https://github.com/mboye/leone/tree/master/leone-webperf/testing/conf
 [Makefile]:https://github.com/mboye/leone/tree/master/Makefile

