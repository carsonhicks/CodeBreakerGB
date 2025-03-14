#
# Simple Makefile that compiles all .c and .s files in the same folder
#

# If you move this project you can change the directory 
# to match your GBDK root directory (ex: GBDK_HOME = "C:/GBDK/"
ifndef GBDK_HOME
	GBDK_HOME = /Users/carson/Documents/gbdk/
endif

LCC = $(GBDK_HOME)bin/lcc 
hUGEDriver = $(GBDK_HOME)lib/gb/hUGEDriver.lib

GBDK_DEBUG = ON
ifdef GBDK_DEBUG
	LCCFLAGS += -debug -v
endif

# You can set the name of the .gb ROM file here
PROJECTNAME    = CodeBreaker

BINS	    = $(PROJECTNAME).gb
CSOURCES   := $(wildcard *.c)
ASMSOURCES := $(wildcard *.s)

all:	$(BINS)

compile.bat: Makefile
	@echo "REM Automatically generated from Makefile" > compile.bat
	@make -sn | sed y/\\//\\\\/ | sed s/mkdir\ \-p/mkdir/ | grep -v make >> compile.bat

# Compile and link all source files in a single call to LCC
$(BINS):	$(CSOURCES) $(ASMSOURCES)
	$(LCC) $(LCCFLAGS) -I../include -Wl-l$(hUGEDriver) -o $@ $(CSOURCES) $(ASMSOURCES)

clean:
	rm -f *.o *.lst *.map *.gb *.ihx *.sym *.cdb *.adb *.asm *.noi *.rst

