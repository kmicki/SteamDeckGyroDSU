BINDIR = bin
DEBUGDIR = $(BINDIR)/debug
RELEASEDIR =  $(BINDIR)/release
EXENAME = sdgyrodsu

OBJDIR = obj
OBJDEBUGDIR = $(OBJDIR)/debug
OBJRELEASEDIR = $(OBJDIR)/release

RELEASEPATH = $(RELEASEDIR)/$(EXENAME)
DEBUGPATH = $(DEBUGDIR)/$(EXENAME)

SRCDIR = src

SRCEXT = cpp
OBJEXT = o

HEADERDIR = inc

CPPSTD = c++2a
ADDLIBS = -pthread -lncurses

COMMONPARS = -std=$(CPPSTD) $(ADDLIBS) -I $(HEADERDIR)
RELEASEPARS = $(COMMONPARS) -O3
DEBUGPARS = $(COMMONPARS)

CC = g++

sources := $(wildcard $(SRCDIR)/*.cpp) $(wildcard $(SRCDIR)/*/*.cpp) $(wildcard $(SRCDIR)/*/*/*.cpp) $(wildcard $(SRCDIR)/*/*/*/*.cpp)
debugobjects := $(subst $(SRCDIR)__, $(OBJDEBUGDIR)/,$(subst /,__,$(sources:.$(SRCEXT)=.$(OBJEXT))))
releaseobjects := $(subst $(SRCDIR)__, $(OBJRELEASEDIR)/,$(subst /,__,$(sources:.$(SRCEXT)=.$(OBJEXT))))

.SILENT:

.PHONY: release
release: $(RELEASEPATH)

.PHONY: debug
debug: $(DEBUGPATH)

$(RELEASEPATH): $(OBJRELEASEDIR) $(RELEASEDIR) $(releaseobjects)
	echo "Linking into $@"
	$(CC) $(filter %.$(OBJEXT),$^) $(RELEASEPARS) -o $@

$(DEBUGPATH): $(OBJDEBUGDIR) $(DEBUGDIR) $(debugobjects)
	echo "Linking into $@"
	$(CC) $(filter %.$(OBJEXT),$^) $(DEBUGPARS) -o $@

.SECONDEXPANSION:

$(debugobjects): $(OBJDEBUGDIR)/%.$(OBJEXT): $$(subst __,/,$(SRCDIR)/%.$(SRCEXT)) \
$$(shell g++ -M -I $(HEADERDIR)/ $$(subst __,/,$$(subst .$(OBJEXT),.$(SRCEXT),$$(subst $(OBJDEBUGDIR)/,$(SRCDIR)/,$$@))) | sed 's/.*\.o://' | sed 's/ \\//g' | tr '\n' ' ')
	echo "Building $< into $@"
	$(CC) $< -c $(DEBUGPARS) -o $@

$(releaseobjects): $(OBJRELEASEDIR)/%.$(OBJEXT): $$(subst __,/,$(SRCDIR)/%.$(SRCEXT)) \
$$(shell g++ -M -I $(HEADERDIR)/ $$(subst __,/,$$(subst .$(OBJEXT),.$(SRCEXT),$$(subst $(OBJRELEASEDIR)/,$(SRCDIR)/,$$@))) | sed 's/.*\.o://' | sed 's/ \\//g' | tr '\n' ' ')
	echo "Building $< into $@"
	$(CC) $< -c $(RELEASEPARS) -o $@

$(OBJRELEASEDIR) $(OBJDEBUGDIR): | $(OBJDIR)
	mkdir $@

$(OBJDIR):
	mkdir $@

$(RELEASEDIR) $(DEBUGDIR): | $(BINDIR)
	mkdir $@

$(BINDIR):
	mkdir $@

clean: dbgclean relclean

relclean:
	rm -f $(OBJRELEASEDIR)/*.$(OBJEXT)
	rm -f $(RELEASEPATH)
	
dbgclean:
	rm -f $(OBJDEBUGDIR)/*.$(OBJEXT)
	rm -f $(DEBUGPATH)