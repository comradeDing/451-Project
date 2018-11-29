CC=g++
VER=c++11

BDIR=build
SDIR=src
PDIR=progs

MSTEXE=MASTER
MSTSRC=master.cpp

P1EXE=PROG1
P1SRC=program1.cpp

P2EXE=PROG2
P2SRC=program2.cpp

P3EXE=PROG3
P3SRC=program3.cpp

init:
	mkdir $(BDIR)

clean:
	rm -r $(BDIR)

buildm:
	$(CC) -std=$(VER) -o $(BDIR)/$(MSTEXE) $(SDIR)/$(MSTSRC)

build1:
	$(CC) -std=$(VER) -o $(BDIR)/$(P1EXE) $(SDIR)/$(PDIR)/$(P1SRC)

build2:
	$(CC) -std=$(VER) -o $(BDIR)/$(P2EXE) $(SDIR)/$(PDIR)/$(P2SRC)

build3:
	$(CC) -std=$(VER) -o $(BDIR)/$(P3EXE) $(SDIR)/$(PDIR)/$(P3SRC)

buildall:
	make init buildm build1 build2 build3

run:
	./$(BDIR)/$(MSTEXE) $(A) $(B)

remake:
	make clean buildall