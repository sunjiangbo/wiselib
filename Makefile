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

export APP_SRC=sdcard_benchmark.cpp
export BIN_OUT=sdcard_benchmark

include ../Makefile
