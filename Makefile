BUILDROOT = /home/wayne/src/a7d65/buildroot-at91/output
CROSS = $(BUILDROOT)/host/bin/arm-buildroot-linux-gnueabihf-
SYSROOT = $(BUILDROOT)/host/arm-buildroot-linux-gnueabihf/sysroot

CXX = $(CROSS)g++
CC = $(CROSS)gcc

PKG_CONFIG = PKG_CONFIG_PATH=$(SYSROOT)/usr/lib/pkgconfig:$(SYSROOT)/usr/share/pkgconfig \
             PKG_CONFIG_SYSROOT_DIR=$(SYSROOT) \
             $(BUILDROOT)/host/bin/pkg-config

EGT_CFLAGS := $(shell $(PKG_CONFIG) --cflags libegt)
EGT_LIBS := $(shell $(PKG_CONFIG) --libs libegt)

KWS_DIR = $(shell pwd)/..
KWS_INC = -I$(KWS_DIR)/library/src -I$(KWS_DIR)/application
KWS_LIB = -L$(KWS_DIR)/application -lvedyasama

CFLAGS = -O2 --sysroot=$(SYSROOT) $(KWS_INC)
CXXFLAGS = -std=c++17 -O2 --sysroot=$(SYSROOT) $(EGT_CFLAGS) $(KWS_INC)
LDFLAGS = --sysroot=$(SYSROOT) $(EGT_LIBS) $(KWS_LIB) -lasound -lm -lpthread

TARGET = kws_gui
SRCS = main.cpp kws_gui.cpp pulse_widget.cpp
OBJS = $(SRCS:.cpp=.o)
KWS_MODEL_OBJ = vedya_Model_class.o

all: $(TARGET)

$(TARGET): $(OBJS) $(KWS_MODEL_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(KWS_MODEL_OBJ) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(KWS_MODEL_OBJ): $(KWS_DIR)/application/vedya_Model_class.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
