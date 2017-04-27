CXX = clang++
#CXX = g++ 
RM = rm

# ========== Directories ==========
SRCDIR := ./src
OBJDIR := .
BINDIR := .

# ============  Flags  ============
CXXFLAGS = -std=c++11 -Wno-write-strings  -O2


# ============ Sources ============
.PHONY: *.so

# ============ Targets ============

Tetris:	$(SRCDIR)/main.cpp
	@(if [ ! -d $(OBJDIR) ]; then echo "Creating \`$(OBJDIR)\` folder"; mkdir $(OBJDIR); fi)
	@(if [ ! -d $(BINDIR) ]; then echo "Creating \`$(BINDIR)\` folder"; mkdir $(BINDIR); fi)
	$(CXX) -o $(BINDIR)/$@  $^ $(CXXFLAGS) -L/usr/local/lib64 -lglut -lGLU -lGL -lXmu -lXext -lX11 -lm -lpthread


.PHONY: Tetris


