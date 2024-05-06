CC=g++
SOURCES=utils.cpp
OBJDIR=obj
DISTDIR=dist
OBJECTS=$(SOURCES:%.cpp=$(OBJDIR)/%.o)
EXECUTABLE1=server
EXECUTABLE2=client

all: $(DISTDIR) $(OBJDIR) $(EXECUTABLE1) $(EXECUTABLE2) 

$(DISTDIR) $(OBJDIR):
	mkdir -p $@

$(EXECUTABLE1): $(OBJECTS) $(OBJDIR)/server.o
	$(CC) $^ -std=c++17 -o $(DISTDIR)/$@

$(EXECUTABLE2): $(OBJECTS) $(OBJDIR)/client.o
	$(CC) $^ -std=c++17 -o $(DISTDIR)/$@

$(OBJDIR)/%.o: %.cpp
	$(CC) -c $< -std=c++17 -o $@

clean:
	rm -f $(OBJDIR)/*.o 
	rm -f $(DISTDIR)/$(EXECUTABLE1) $(DISTDIR)/$(EXECUTABLE2)
