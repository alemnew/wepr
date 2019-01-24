hurl
---
hURL is a multi-threaded HTTP client which supports persistent connections and request pipelining.

dns-library
---
Library for DNS resolution

playback
---
DNS and HTTP server designed to emulate performance recorded by the Webperf test.

rendering-server
---
Calcualtes the rendering time of a website.

tools
---
Library containing shared functions.

webperf
---
The Web performace test.
Examples of test configuration files are located under [webperf/google.conf]

Check [webperf/webperf.conf] for list of configuration options and their meaning.

parsing-server
---
Parsing server that turns HTML files into a list of files that must be downloaded in order to render a webpage.



How to build
---
Before compiling for MIPS, change OpenWRT build-root paths in main [Makefile].

```sh
    make x86
    make mips
```

The binaries for webperf are located in webperf/x86 and webperf/mips, and for the playback server is located in playback/x86.

 [webperf/webperf.conf]:https://github.com/alemnew/tree/master/webperf/webperf.conf
 [webperf/google.conf]:https://github.com/alemnew/tree/master/webperf/google.conf
 [Makefile]:https://github.com/alemnew/wepr/blobe/master/Makefile

