TARGET := .
CC = gcc
CXX = g++
CFLAGS = -O2 -g -Wall -I./include
CXXFLAGS = $(CFLAGS) -std=c++11

LINKAGE = -lbfd -liberty

OBJ_DIR = obj
SRC = RegFile.cc
OBJ = $(addsuffix .o, $(basename $(SRC)))

vpath %.o $(OBJ_DIR)

.PHONY: init clean cleanall
.NOPARALLEL: all
all: init sim

init:
	@if [ ! -d $(OBJ_DIR) ]; then mkdir $(OBJ_DIR) || exit 1 ; fi

%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $(OBJ_DIR)/$@

%.o: %.cc
	$(CXX) $(CXXFLAGS) $< -c -o $(OBJ_DIR)/$@

sim: $(OBJ) main.cc
	$(CXX) $(CXXFLAGS) $< $(addprefix $(OBJ_DIR)/,$(OBJ)) -o $(TARGET)/sim $(LINKAGE) 

clean:
	rm -rf $(OBJ_DIR)

cleanall: clean
