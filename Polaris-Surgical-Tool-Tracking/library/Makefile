# library

CC=g++
# BUILD_DIR=../build/linux
CFLAGS=-c -fPIC -pthread -std=c++11
LFLAGS=-shared -fPIC

# On Mac, specify the library should be linked using rpath
OS=$(shell uname -s)
ifeq ($(OS),Darwin)
LFLAGS+=-install_name @rpath/$(notdir $(LIB_LIBRARY))
endif

obj_dir ?= $(BUILD_DIR)/obj/library

sources := $(wildcard ./src/*.cpp)
objects = $(sources:%.cpp=$(obj_dir)/%.o)
include_dirs := include src/include
CPPFLAGS += $(addprefix -I ,$(include_dirs))

all: $(LIB_LIBRARY)

# Create a dynamically linked library
$(LIB_LIBRARY): $(objects)
	mkdir -p $(dir $@)
	$(CC) $(LFLAGS) -o$@ $^
	@echo "library build successful!"

# Compile source code into object files
$(objects): $(obj_dir)/%.o: %.cpp
	@echo Compiling $<
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $<

.PHONY: clean
clean:
	@echo "doing clean in library"
	$(RM) -r $(obj_dir)	
	$(RM) -f $(LIB_LIBRARY)	
