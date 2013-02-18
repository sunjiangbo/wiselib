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

export APP_SRC=block_device_mmu_test_app.cpp
export BIN_OUT=block_device_mmu_test_app

include ../Makefile

both_: pc_ ar_

ar_: compile flash run

pc_: pc
	out/pc/./block_device_mmu_test_app

compile:
	make arduino BOARD=mega2560

flash:
	killall screen; make flash-arduino BOARD=mega2560 PORT=/dev/ttyACM0

run:
	killall screen; make monitor-arduino BOARD=mega2560 PORT=/dev/ttyACM0

clean4git:
	rm -f *~; rm -rf out;