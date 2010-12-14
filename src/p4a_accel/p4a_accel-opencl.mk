CC = gcc
LINK       := gcc -fPIC

all: $(EXECUTABLE)
NVIDIA_SDK_DIR=$(HOME)/NVIDIA_GPU_Computing_SDK
SHARED_DIR     := $(NVIDIA_SDK_DIR)/shared
OCL_DIR     := $(NVIDIA_SDK_DIR)/OpenCL/common

# Add runtime:
OBJFILES += p4a_accel.o

p4a_accel.c: $(P4A_ACCEL_DIR)/p4a_accel.c
	ln -s $< $@

CFLAGS = -I.. -I.  -DP4A_ACCEL_CL -DP4A_DEBUG -DP4A_PROFILING -I$(OCL_DIR)/inc -I$(OCL_DIR)/inc/CL -I$(SHARED_DIR)/inc -DUNIX -std=c99

LDFLAGS = -fPIC -L/usr/lib #-L$(SHARED_DIR)/lib -L$(OCL_DIR)/lib

#LDLIBS = -lcudart -lcutil_x86_64
#LDLIBS =  $(OCL_DIR)/lib/liboclUtil_x86_64.a $(SHARED_DIR)/lib/libshrutil_x86_64.a -lOpenCL
#LDLIBS =  -loclUtil_x86_64 -lshrutil_x86_64 -lOpenCL
LDLIBS =  -lOpenCL 

# New default rule to compile OpenCL source files:
%.o: %.c
	$(CC) -c $(CFLAGS) $<

$(EXECUTABLE): $(OBJFILES)
	$(LINK) $(CFLAGS) -o $@ $(OBJFILES) $(LDLIBS) $(LDFLAGS)

clean::
	rm -f $(EXECUTABLE) $(OBJFILES)

