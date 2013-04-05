# ----------------------------------------
# Environment variable WISELIB_PATH needed
# ----------------------------------------

all: shawn
# all: scw_msb
# all: contiki_msb
# all: contiki_micaz
# all: isense
# all: tinyos-tossim
# all: tinyos-micaz

export APP_SRC=doms_test.cpp
export BIN_OUT=doms_test


include ../Makefile

compile:
	make arduino BOARD=mega2560

write:
	killall screen; make flash-arduino BOARD=mega2560 PORT=/dev/ttyACM0

show:
	make monitor-arduino BOARD=mega2560 PORT=/dev/ttyACM0	
