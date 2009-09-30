#
# Makefile for the project
#

binaries = vmcontrol

CC = gcc
CC_ARGS = -Wall -pedantic -g -std=c99

CPP = g++
CPP_ARGS = -Wall -g

VIX_INC = /usr/include/vmware-vix
VIX_LINK = -lvixAllProducts -ldl

all: $(binaries)

vmcontrol: vmcontrol.o
	$(CPP) $(CPP_ARGS) -o vmcontrol vmcontrol.o $(VIX_LINK)

vmcontrol.o: vmcontrol.cpp
	$(CPP) $(CPP_ARGS) -I$(VIX_INC) -c vmcontrol.cpp

.PHONY: clean

clean: 
	rm -f *.o *.ls *.hex *.class $(binaries)
