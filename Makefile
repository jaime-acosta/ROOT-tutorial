.EXPORT_ALL_VARIABLES:

.PHONY: clean all docs

BIN_DIR = ./bin
LIB_DIR = ./lib

ROOTCFLAGS  := $(shell root-config --cflags)
ROOTLIBS    := $(shell root-config --libs)
ROOTGLIBS   := $(shell root-config --glibs)
ROOTINC     := -I$(shell root-config --incdir)

CPP         = g++
CFLAGS	    = -Wall -Wno-long-long -g -O3 $(ROOTCFLAGS) -fPIC -D_FILE_OFFSET_BITS=64 -MMD

INCLUDES    = -I./inc
BASELIBS    = -lm $(ROOTLIBS) $(ROOTGLIBS) -L$(LIB_DIR) -lXMLParser
LIBS  	    =  $(BASELIBS)

SWITCH = -DWITHSIM

LFLAGS	    = -g -fPIC -shared
CFLAGS 	    += -Wl,--no-as-needed $(SWITCH)
LFLAGS 	    += -Wl,--no-as-needed 
CFLAGS 	    += -Wno-unused-variable -Wno-unused-but-set-variable -Wno-write-strings

CLICFLAGS   = -g2 -O2 -fPIC
CLILFLAGS   = -g -fPIC -shared -Wl,--no-as-needed 

LIB_O_FILES = build/Event.o build/EventDictionary.o

USING_ROOT_6 = $(shell expr $(shell root-config --version | cut -f1 -d.) \>= 6)
ifeq ($(USING_ROOT_6),1)
	EXTRAS =  EventDictionary_rdict.pcm
endif

all: $(LIB_DIR)/libEvent.so $(EXTRAS) Analyze

exe: Analyze

Analyze: Analyze.cc $(LIB_DIR)/libEvent.so $(LIB_O_FILES)
	@echo "Compiling $@"
	@$(CPP) $(CFLAGS) $(INCLUDES) $< $(LIBS) $(LIB_O_FILES) -o $(BIN_DIR)/$@ 

$(LIB_DIR)/libEvent.so: $(LIB_O_FILES)
	@echo "Making $@"
	@$(CPP) $(LFLAGS) -o $@ $^ -lc

build/Event.o: src/Event.cc inc/Event.hh
	@echo "Compiling $@"
	@mkdir -p $(dir $@)
	@$(CPP) $(CFLAGS) $(INCLUDES) -c $< -o $@ 

build/%.o: src/%.cc inc/%.hh
	@echo "Compiling $@"
	@mkdir -p $(dir $@)
	@$(CPP) $(CFLAGS) $(INCLUDES) -c $< -o $@ 

build/%Dictionary.o: build/%Dictionary.cc
	@echo "Compiling $@"
	@mkdir -p $(dir $@)
	@$(CPP) $(CFLAGS) $(INCLUDES) -fPIC -c $< -o $@

build/%Dictionary.cc: inc/%.hh inc/%LinkDef.h
	@echo "Building $@"
	@mkdir -p $(dir $@)
	@rootcint -f $@ -c $(INCLUDES) $(ROOTCFLAGS) $(SWITCH) $(notdir $^)

build/%Dictionary_rdict.pcm: build/%Dictionary.cc
	@echo "Confirming $@"
	@touch $@

%Dictionary_rdict.pcm: build/%Dictionary_rdict.pcm 
	@echo "Placing $@"
	@cp build/$@ $(BIN_DIR)


doc:	doxy-config
	doxygen doxy-config


clean:
	@echo "Cleaning up"
	@rm -rf build doc
	@rm -f inc/*~ src/*~ *~
