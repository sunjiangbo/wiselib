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

export APP_SRC=block_memory_management_unit_test_app.cpp
export BIN_OUT=block_memory_management_unit_test_app

include ../Makefile

# all: pc_

both_: pc_ ar_

ar_: compile flash run

pc_: pc
	out/pc/./block_memory_management_unit_test_app

debug: 
	make pc PC_COMPILE_DEBUG=1

#debug:
#	LANG=C make debugCompile 2>&1|/home/marco/IBR_HiWi/wiselib/util/compile/detemplate -w -t0 -a0

debug_: debug
	gdb out/pc/./block_memory_management_unit_test_app 

gdb:	
	gdb out/pc/./block_memory_management_unit_test_app

runpc:
	out/pc/./block_memory_management_unit_test_app

compile:
	make arduino BOARD=mega2560

flash:
	killall screen; make flash-arduino BOARD=mega2560 PORT=/dev/ttyACM0

run:
	killall screen; make monitor-arduino BOARD=mega2560 PORT=/dev/ttyACM0

clean4git:
	rm -f *~; rm -rf out;
