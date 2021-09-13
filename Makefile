DIR_INC := ./inc
DIR_SRC := ./src
DIR_OBJ := ./obj

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
INCLUDE_DIRS ?= 
LIBRARY_DIRS ?=

SRC := $(wildcard ${DIR_SRC}/*.cpp)
OBJ := $(patsubst %.cpp,${DIR_OBJ}/%.o,$(notdir ${SRC}))

TARGET := ST_BarcodeMap-0.0.1

BIN_TARGET := ${TARGET}


ifndef asserts
    asserts=1
endif

ifneq ($(asserts),1)
     override CXXFLAGS+= -DNDEBUG
endif

ifeq ($(print_debug),1)
     override CXXFLAGS+= -DPRINT_DEBUG=1
endif

ifeq ($(print_debug_decoding),1)
     override CXXFLAGS+= -DPRINT_DEBUG_DECODING=1
endif


CXX = g++
CXXFLAGS := -std=c++11 -g -O3 -I${DIR_INC} -mssse3 -I./ -I./common $(foreach includedir,$(INCLUDE_DIRS),-I$(includedir)) ${CXXFLAGS} \
$(call cc-option,-flto=jobserver,-flto) -march=native -mtune=native
LIBS := -lz -lpthread -lhdf5 -lboost_serialization -fopenmp -lrt
LD_FLAGS := $(foreach librarydir,$(LIBRARY_DIRS),-L$(librarydir)) $(LIBS) $(LD_FLAGS)


${BIN_TARGET}:${OBJ}
	$(CXX) $(OBJ) -o $@ $(LD_FLAGS)

${DIR_OBJ}/%.o:${DIR_SRC}/%.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS)

.PHONY:clean
clean:
	rm obj/*.o
	rm $(TARGET)

make_obj_dir:
	@if test ! -d $(DIR_OBJ) ; \
	then \
		mkdir $(DIR_OBJ) ; \
	fi

install:
	install $(TARGET) $(BINDIR)/$(TARGET)
	@echo "Installed."
