#   ----------------------------------------------------------------------------
#  @file   Makefile
#
#  @path   
#
#  @desc   Makefile for ora test lib
#
#  @ver    1.0
#   ----------------------------------------------------------------------------

#   ----------------------------------------------------------------------------
#   Included defined variables
#   ----------------------------------------------------------------------------
include ../../Rules.make

#   ----------------------------------------------------------------------------
#   Variables passed in externally
#   ----------------------------------------------------------------------------
PLATFORM ?=
ARCH     ?=
MINI     ?=
CROSS_COMPILE   ?=
SDK_PATH_TARGET ?=

#   ----------------------------------------------------------------------------
#   Name of the Linux compiler
#   ----------------------------------------------------------------------------
BIN := fastsetupd
OUT ?= build

CC  := $(CROSS_COMPILE)gcc
CXX := $(CROSS_COMPILE)g++

INCLUDES := -I$(SDK_PATH_TARGET)usr/include
INCLUDES += -I../include -I./Common

LD_FLAGS := -L$(SDK_PATH_TARGET)usr/lib
LD_FLAGS += -L../lib
LD_FLAGS += -lpthread -lrt -lm -lcrypt
LD_FLAGS += -loraconfig -loralog -loraipc -lorasys

CFLAGS   += -fPIC -g $(INCLUDES)
CXXFLAGS += $(CFLAGS)

SOURCE_C	 := $(wildcard *.c)
SOURCE_CPP := $(wildcard ./Common/*.cpp *.cpp)

OBJS := $(patsubst %.c, %.o, $(SOURCE_C))
OBJS += $(patsubst %.cpp, %.o, $(SOURCE_CPP))
OBJS := $(patsubst %, $(OUT)/%, $(OBJS))

all: $(OBJS)
	@$(CXX) -o $(OUT)/$(BIN) $(OBJS) $(LD_FLAGS)
	@echo "==> Build [$(OUT)/$(BIN)] Finished!!! <=="

clean:
	-@rm $(OUT) -rf

$(OUT)/%.d: %.cpp
	@mkdir -p $(OUT)
	@mkdir -p `dirname $@`
	@$(CXX) $(CXXFLAGS) $< -MM -o $@
	@sed -i 's#\(.*\)\.o[ :]*#'$(OUT)'/\1.o $@ : #g' $@

$(OUT)/%.o: %.cpp $(OUT)/%.d
	@mkdir -p $(OUT)
	@echo "-->compiling $< ..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(patsubst %.c, $(OUT)/%.d, $(SOURCE_C))
-include $(patsubst %.cpp, $(OUT)/%.d, $(SOURCE_CPP))

install:
	@if [[ ! -d $(DESTDIR) ]]; then \
		echo "The target filesystem directory doesn't exist."; \
		exit 1; \
	fi
	@mkdir -p $(DESTDIR)/usr/local/bin
	@cp $(OUT)/$(BIN) $(DESTDIR)/usr/local/bin/
	@install -c $(OUT)/$(BIN) $(DESTDIR)/usr/local/bin/
	@install -c -m 644 $(BIN).service $(DESTDIR)/etc/systemd/system/multi-user.target.wants

