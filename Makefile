DIR_INC := ./inc
DIR_SRC := ./src
DIR_OBJ := ./obj

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
INCLUDE_DIRS ?= 
LIBRARY_DIRS ?=

SRC := $(wildcard ${DIR_SRC}/*.cpp)
OBJ := $(patsubst %.cpp,${DIR_OBJ}/%.o,$(notdir ${SRC}))


SRC2 := $(wildcard ${DIR_SRC}/*.c)
OBJ += $(patsubst %.c,${DIR_OBJ}/%.o,$(notdir ${SRC2}))

#$(info SRC $(SRC))
#$(info SRC2 $(SRC2))
#$(info OBJ $(OBJ))


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


CXX = mpigxx

#CXXFLAGS := -std=c++11 -I./ -I./common -march=native -mtune=native -g -O3 -w -fopenmp

#CXXFLAGS := -std=c++11 -g -O3   -I./ -I./common $(foreach includedir,$(INCLUDE_DIRS),-I$(includedir)) ${CXXFLAGS} \
$(call cc-option,-flto=jobserver,-flto) -march=native -mtune=native -fopenmp
CXXFLAGS := -std=c++11 -I. -Icommon -w -Wextra -Weffc++ -Wpedantic -Wundef -Wuseless-cast -Wconversion -Wshadow -Wdisabled-optimization -Wparentheses -Wpointer-arith   -O3 -flto=jobserver -march=native -mtune=native -g -D_POSIX_C_SOURCE=200809L -D_FILE_OFFSET_BITS=64 -fopenmp

CXX2 = mpigcc
CXXFLAGS2 := -g -O3 -w -Wextra -Wno-unknown-pragmas -Wcast-qual

LIBS := -std=c++11 -I. -Icommon -w -Wextra -Weffc++ -Wpedantic -Wundef -Wuseless-cast -Wconversion -Wshadow -Wdisabled-optimization -Wparentheses -Wpointer-arith   -O3 -flto=jobserver -march=native -mtune=native -g -D_POSIX_C_SOURCE=200809L -D_FILE_OFFSET_BITS=64 -lz -lpthread -lhdf5 -lboost_serialization -fopenmp -lrt -lm  -lrt -ldeflate

#LIBS := -lz -lpthread -lhdf5 -lboost_serialization -fopenmp -lrt -lm -ldeflate
#LD_FLAGS := $(foreach librarydir,$(LIBRARY_DIRS),-L$(librarydir)) $(LIBS) $(LD_FLAGS)
LD_FLAGS := $(LIBS)




${BIN_TARGET}:${OBJ}
	$(CXX) $(OBJ) -o $@ $(LD_FLAGS)

${DIR_OBJ}/%.o:${DIR_SRC}/%.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS)


${DIR_OBJ}/%.o:${DIR_SRC}/%.c
	$(CXX2) $(CXXFLAGS2) -c $< -o $@

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
