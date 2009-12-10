#
# Makefile for the project
#

binary = vmcontrol

# Install variables
BIN = $(DESTDIR)/usr/sbin
CONFIG = $(DESTDIR)/etc/vmware/vmcontrol

VERSION = 0.9

DIST = vmcontrol-$(VERSION).tar
DISTGZ = $(DIST).gz

CC = gcc
CC_ARGS = -Wall -pedantic -g -std=c99

CPP = g++
CPP_ARGS = -Wall -g

VIX_INC = /usr/include/vmware-vix
VIX_LINK = -lvixAllProducts -ldl 

CPULIMIT_LINK = -lrt

DAEMON_LINK = -ldaemon

all: $(binary)

vmcontrol: vmcontrol.o cpulimit.o vixint.o util.o 
	$(CPP) $(CPP_ARGS) -o vmcontrol vmcontrol.o cpulimit.o vixint.o util.o $(VIX_LINK) $(CPULIMIT_LINK) $(DAEMON_LINK)

vmcontrol.o: vmcontrol.cpp cpulimit.h vixint.h util.h
	$(CPP) $(CPP_ARGS) -I$(VIX_INC) -c vmcontrol.cpp

cpulimit.o: cpulimit.c cpulimit.h 
	$(CPP) $(CPP_ARGS) -c cpulimit.c

vixint.o: vixint.cpp vixint.h util.h
	$(CPP) $(CPP_ARGS) -I$(VIX_INC) -c vixint.cpp

util.o: util.cpp util.h
	$(CPP) $(CPP_ARGS) -c util.cpp

parse: parse.cpp
	$(CPP) $(CPP_ARGS) -o parse parse.cpp

.PHONY: clean test-up test

clean: 
	rm -f *.o *.ls *.hex *.class $(binary)

install: all
	install -d $(BIN) $(CONFIG)
	install ./vmcontrol $(BIN)
	install -m640 ./config $(CONFIG)

test: vmcontrol
	sudo ./vmcontrol -n -u

dist:
	-cd .. && rm -f $(DISTGZ)
	cd .. && tar cvf $(DIST) vmcontrol-$(VERSION)/
	cd .. && gzip $(DIST)
