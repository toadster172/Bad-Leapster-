CC = cc
CXX = c++
CFLAGS = -std=c23
CXXFLAGS =
LIBS = -lelf
ODIR=obj
_CXXOBJ = binToRib.o
_COBJ = lfElf.o
CXXOBJ  = $(patsubst %,$(ODIR)/%,$(_CXXOBJ))
COBJ  = $(patsubst %,$(ODIR)/%,$(_COBJ))
default: binToRib

$(COBJ): $(ODIR)/%.o: %.c
	mkdir -p $(ODIR)
	$(CC) -c -o $@ $< $(CFLAGS)
$(CXXOBJ): $(ODIR)/%.o: %.cpp
	mkdir -p $(ODIR)
	$(CXX) -c -o $@ $< $(CXXFLAGS)
binToRib: $(COBJ) $(CXXOBJ)
	$(CXX) -o $@ $^ $(LIBS)
clean:
	rm -rf $(ODIR)/*
	rm ./binToRib
