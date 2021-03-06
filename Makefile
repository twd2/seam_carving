include config.make

ifdef DEBUG
DBG = -g
else
DBG = -O2
endif

ifeq ($(shell uname),Linux)
	# compiler
	CC = g++
	INC_DIR = -I include
	CC_FLAGS = $(DBG) -Wall -fno-strict-aliasing -std=gnu++11 $(INC_DIR)

	# linker
	LD = g++
	LIB_DIR = -L lib/linux
	LD_LIBS = -lstdc++ -lc -lm -ldl
	LD_FLAGS = $(LIB_DIR)
else

ifeq ($(shell uname),Darwin)
	# compiler
	INC_DIR = -I include
	CC_FLAGS = $(DBG) -Wall -fno-strict-aliasing -std=gnu++11 $(INC_DIR)

	# linker
	LD = cc
	LIB_DIR = -L./lib/osx
	LD_LIBS = -lc++ -lm -lSystem -lopencv_core -lopencv_imgproc -lopencv_imgcodecs
	LD_FLAGS = $(LIB_DIR)
else
	# Unsupported operating system.
	CC = echo && echo "******** Unsupported operating system! ********" && echo && exit 1 ||
endif

endif

.PHONY: all
all: main
	./main test.png

main: main.o $(OBJECTS)
	$(LD) $(LD_FLAGS) -o $@ $^ $(LD_LIBS)
	install_name_tool -add_rpath '.' main

main.o: main.cpp $(HEADERS)
	$(CC) $(CC_FLAGS) -o $@ -c $<

%.o: %.cpp $(HEADERS)
	$(CC) $(CC_FLAGS) -o $@ -c $<

.PHONY: clean
clean:
	-$(RM) *.o main test.png
