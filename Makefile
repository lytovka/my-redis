CC=gcc
SOURCES=utils.c
OBJDIR=obj
DISTDIR=dist
OBJECTS=$(SOURCES:%.c=$(OBJDIR)/%.o)
EXECUTABLE1=server
EXECUTABLE2=client

all: $(DISTDIR) $(OBJDIR) $(EXECUTABLE1) $(EXECUTABLE2) 

$(DISTDIR) $(OBJDIR):
	mkdir -p $@

$(EXECUTABLE1): $(OBJECTS) $(OBJDIR)/server.o
	$(CC) $^ -o $(DISTDIR)/$@

$(EXECUTABLE2): $(OBJECTS) $(OBJDIR)/client.o
	$(CC) $^ -o $(DISTDIR)/$@

$(OBJDIR)/%.o: %.c
	$(CC) -c $< -o $@

clean:
	rm -f $(OBJDIR)/*.o 
	rm -f $(DISTDIR)/$(EXECUTABLE1) $(DISTDIR)/$(EXECUTABLE2)
