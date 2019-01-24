#!/bin/bash
#Block all .fi domain servers except e.fi
iptables -F
iptables -I OUTPUT -p udp -d 193.166.4.1 --dport 53 -j DROP
iptables -I OUTPUT -p udp -d 2001:708:10:53::53 --dport 53 -j DROP
iptables -I OUTPUT -p udp -d 194.146.106.26 --dport 53 -j DROP
iptables -I OUTPUT -p udp -d 2001:67c:1010:6::53 --dport 53 -j DROP
iptables -I OUTPUT -p udp -d 156.154.100.26 --dport 53 -j DROP
iptables -I OUTPUT -p udp -d 2001:502:ad09::26 --dport 53 -j DROP
iptables -I OUTPUT -p udp -d 77.72.229.253 --dport 53 -j DROP
iptables -I OUTPUT -p udp -d 2a01:3f0:0:302::53 --dport 53 -j DROP
iptables -I OUTPUT -p udp -d 87.239.127.198 --dport 53 -j DROP
iptables -I OUTPUT -p udp -d 156.154.101.26 --dport 53 -j DROP
iptables -I OUTPUT -p udp -d 156.154.102.26 --dport 53 -j DROP
iptables -I OUTPUT -p udp -d 156.154.103.26 --dport 53 -j DROP
iptables -I OUTPUT -p udp -d 2001:502:2eda::26 --dport 53 -j DROP

