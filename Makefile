CXX ?= g++
ROOT_CFLAGS := $(shell root-config --cflags)
ROOT_LIBS := $(shell root-config --libs)

CPPFLAGS ?= -Ilib/NuIO/include -Ilib/NuPOT/include
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra $(ROOT_CFLAGS) -fPIC
LDFLAGS ?= $(ROOT_LIBS) -lsqlite3

OBJDIR := build/obj
LIBDIR := lib
BINDIR := bin

NUIO_SRC := lib/NuIO/src/StageResultIO.cxx
NUPOT_SRC := lib/NuPOT/src/BeamRunDB.cxx
CONDER_SRC := executables/nuIOcondenser/src/main.cxx

NUIO_OBJ := $(OBJDIR)/StageResultIO.o
NUPOT_OBJ := $(OBJDIR)/BeamRunDB.o
CONDER_OBJ := $(OBJDIR)/nuIOcondenser.o

LIBNUIO := $(LIBDIR)/libNuIO.so
LIBNUPOT := $(LIBDIR)/libNuPOT.so

NUIOCONDENSER := $(BINDIR)/nuIOcondenser.exe

.PHONY: all clean dirs

all: dirs $(LIBNUIO) $(LIBNUPOT) $(NUIOCONDENSER)

dirs:
	@mkdir -p $(OBJDIR) $(LIBDIR) $(BINDIR)

$(NUIO_OBJ): $(NUIO_SRC)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(NUPOT_OBJ): $(NUPOT_SRC)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(CONDER_OBJ): $(CONDER_SRC)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(LIBNUIO): $(NUIO_OBJ)
	$(CXX) -shared $< -o $@

$(LIBNUPOT): $(NUPOT_OBJ)
	$(CXX) -shared $< -o $@

$(NUIOCONDENSER): $(CONDER_OBJ) $(LIBNUIO) $(LIBNUPOT)
	$(CXX) $(CONDER_OBJ) -L$(LIBDIR) -lNuIO -lNuPOT $(LDFLAGS) -o $@

clean:
	rm -rf $(OBJDIR) $(LIBDIR) $(BINDIR)
