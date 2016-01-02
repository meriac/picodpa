PREFIX=
SCOPE:=ps3000a
IPS_VER:=1.1
SOURCES:=main.cpp
TARGET_BIN:=picodpa
OPT:=-O2

#
# derived settings
#
PICO_ROOT:=/opt/picoscope
PICO_LIB:=$(PICO_ROOT)/lib
PICO_INC:=$(PICO_ROOT)/include/lib$(SCOPE)-$(IPS_VER)/
OBJECTS:=$(SOURCES:.cpp=.o)
#
# tools used
#
GCC:=$(PREFIX)gcc
#
# compiler / linker settings
#
COMMON_FLAGS:=$(OPT)
CPPFLAGS:= $(COMMON_FLAGS) -Wall -Werror -I$(PICO_INC)
LDFLAGS:=-L$(PICO_ROOT)/lib -l$(SCOPE)

all: $(TARGET_BIN)

$(TARGET_BIN) : $(OBJECTS)
	$(CXX) $(COMMON_FLAGS) $(LDFLAGS) $^ -o $@

clean:
	rm -f $(OBJECTS) $(TARGET_BIN)
