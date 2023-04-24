CXX := g++
VPATH := ./:../
#CFLAGS += -std=c++17 -rdynamic -Wl,--no-as-needed -Wno-deprecated -Wall -Werror -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -O3 #-O2 -O
CFLAGS += -Wall -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE #-O3 #-O2 -O
DEBUG := -DFIFO_DEBUG_VERBOSE -DFIFO_ERROR_VERBOSE -O3
#FIFO_LIB := -L../build/lib -lshm.r1.0.0 #../build/lib/libshm.r1.0.0.a
HEADER := -I../include
FIFO_TARGET := test
FIFO_SRC := $(wildcard test_*.cc ../src/*.cc)
#FIFO_SRC := $(wildcard *.cc)
FIFO_OBJ := $(FIFO_SRC:%.cc=%.o)

all: $(FIFO_TARGET)

$(FIFO_OBJ): %.o:%.cc
	$(CXX) -c $< -o $@ $(DEBUG) $(CFLAGS) $(HEADER) $(TEST)


$(FIFO_TARGET): $(FIFO_OBJ)
	$(CXX) -o $(FIFO_TARGET) $(FIFO_OBJ) $(LIB) $(FIFO_LIB) $(DEBUG)
	ln -s $(FIFO_TARGET) test_pop
	ln -s $(FIFO_TARGET) test_push

clean:
	rm -rf $(FIFO_OBJ) $(FIFO_TARGET)
	rm -rf test_pop test_push

.PHONY: all




