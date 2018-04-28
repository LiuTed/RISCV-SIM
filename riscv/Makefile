CC = g++
CFLAGS = -std=c++11 -O2
LINKAGE = -lbfd -liberty

OBJ_DIR = build
OBJ = mem.o regfile.o machine.o stats.o cache.o branch_pred.o
OBJ_DB = $(patsubst %.o, %-db.o, $(OBJ))
OBJ_DBG = $(patsubst %.o, %-dbg.o, $(OBJ))
HDR = mem.h regfile.h machine.h stats.h cache.h instr.h utli.h branch_pred.h

.PHONY: clean, cleanall, prepare

vpath %.o $(OBJ_DIR)

all: prepare sim sim-db sim-dbg

prepare:
	@if [ ! -d $(OBJ_DIR) ]; then mkdir $(OBJ_DIR) || exit 1 ; fi

sim: $(OBJ) main.cpp
	$(CC) $(CFLAGS) main.cpp $(foreach O, $(OBJ), $(OBJ_DIR)/$(O)) -o $@ $(LINKAGE)

sim-db: $(OBJ_DB) main.cpp
	$(CC) $(CFLAGS) main.cpp $(foreach O, $(OBJ_DB), $(OBJ_DIR)/$(O)) -o $@ $(LINKAGE) -DSDB

sim-dbg: $(OBJ_DBG) main.cpp
	$(CC) $(CFLAGS) main.cpp $(foreach O, $(OBJ_DBG), $(OBJ_DIR)/$(O)) -o $@ $(LINKAGE) -DSDB -DDEBUG

$(OBJ): %.o: %.cpp $(HDR)
	$(CC) $(CFLAGS) $< -c -o $(OBJ_DIR)/$@

$(OBJ_DB): %-db.o: %.cpp $(HDR)
	$(CC) $(CFLAGS) $< -c -o $(OBJ_DIR)/$@ -DSDB

$(OBJ_DBG): %-dbg.o: %.cpp $(HDR)
	$(CC) $(CFLAGS) $< -c -o $(OBJ_DIR)/$@ -DSDB -DDEBUG

clean:
	rm -f $(OBJ_DIR)/*

cleanall: clean
	rm -f sim sim-db sim-dbg
