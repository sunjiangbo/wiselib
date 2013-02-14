# ----------------------------------------
# Environment variable WISELIB_PATH needed
# ----------------------------------------

# all: shawn
# all: scw_msb
# all: contiki_msb
# all: contiki_micaz
# all: isense
# all: tinyos-tossim
# all: tinyos-micaz

export APP_SRC=bdmmu_test_app.cpp
export BIN_OUT=bdmmu_test_app

include ../Makefile

all: compile flash run

compile:
	make arduino BOARD=mega2560

flash:
	killall screen; make flash-arduino BOARD=mega2560 PORT=/dev/ttyACM0

run:
	killall screen; make monitor-arduino BOARD=mega2560 PORT=/dev/ttyACM0
