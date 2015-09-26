# Arduino Make file. Refer to https://github.com/sudar/Arduino-Makefile

ARDMK_DIR = /usr/local/opt/arduino-mk

# if you have placed the alternate core in your sketchbook directory, then you can just mention the core name alone.
ALTERNATE_CORE = attiny
# If not, you might have to include the full path.
ALTERNATE_VAR_PATH = /Users/doug/Documents/Arduino/hardware/attiny
ALTERNATE_CORE_PATH = /Users/doug/Documents/Arduino/hardware/attiny

BOARD_TAG = attiny84
ISP_PROG = usbtiny

AVRDUDE_OPTS = -q -V -B 250

include $(ARDMK_DIR)/Arduino.mk

# !!! Important. You have to use make ispload to upload when using ISP programmer
