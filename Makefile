## Build a project and combine built executable into binary package together with additional files
## Restrictions: 
##		- one generated binary file (executable)
##		- single extension of compiled source files
##		- compilation with gcc or compatible

# Configuration

# 	Main directories

# 		dir with source files
SRCDIR = src
# 		dir with header files
HEADERDIR = inc
# 		main dir for object files
OBJDIR = obj
# 		main dir for binaries		
BINDIR = bin
# 		dir with files to be included in the binary package		
PKGDIR = pkg
# 		dir for binary package
PKGBINDIR = pkgbin

#	Names

# 		main name for release build artifacts
RELEASE = release
# 		main name for debug build artifacts
DEBUG = debug
# 		main name for the application/executable
EXENAME = sdgyrodsu
# 		main name for the binary package
PKGNAME = SteamDeckGyroDSUSetup

#	Extensions
# 		extension of source files
SRCEXT = cpp
# 		extension of object files
OBJEXT = o

#	Compiler

# 		Compiler executable
CC = g++
# 		C++ standard parameter
CPPSTD = c++2a
# 		Additional parameters for all builds
ADDPARS =
# 		Additional parameters for release build
ADDRELEASEPARS = -O3
# 		Additional parameters for debug build
ADDDEBUGPARS =
# 		Additional libraries parameters
ADDLIBS = -pthread -lncurses

#	Install

# 		Install script name (has to be in $PKGDIR)
INSTALLSCRIPT = install.sh

#	Prepare

#		Prepare script file path, used to install dependencies with missing files in the system
PREPARESCRIPT = scripts/prepare.sh

#	Dependency check (Steam Deck)
#	Some libraries have to be reinstalled on Steam Deck because haeder files are missing

# 		Check files - these files existence will be checked to determine if 
#		the corresponding dependency is properly installed
DEPENDCHECKFILES := $(firstword $(wildcard /usr/include/c++/*/vector)) /usr/include/errno.h /usr/include/linux/can/error.h /usr/include/ncurses.h

# 		Dependencies - names of packages to install with pacman -S
#		if the corresponding check file is not found
DEPENDENCIES := gcc glibc linux-api-headers ncurses

# Functions

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

# Generated variables

# 	Subdirectories

# 		dir for debug build's objects
OBJDEBUGDIR = $(OBJDIR)/$(DEBUG)
# 		dir for release build's objects
OBJRELEASEDIR = $(OBJDIR)/$(RELEASE)
# 		dir for debug build's binaries
DEBUGDIR = $(BINDIR)/$(DEBUG)
# 		dir for release build's binaries
RELEASEDIR =  $(BINDIR)/$(RELEASE)
# 		dir for binary package contents
PKGPREPDIR = $(PKGBINDIR)/$(PKGNAME)

# 	File paths

# 		path for debug build's executable
DEBUGPATH = $(DEBUGDIR)/$(EXENAME)
# 		path for release build's executable
RELEASEPATH = $(RELEASEDIR)/$(EXENAME)
# 		inary package file name
PKGBIN = $(PKGNAME).zip
# 		path for binary package
PKGBINPATH = $(PKGBINDIR)/$(PKGBIN)

# 	Compilator parameters

# 		common parameters for a compilator
COMMONPARS = -std=$(CPPSTD) $(GENLIBS) -I $(HEADERDIR) $(ADDPARS)
# 		parameters for release build
RELEASEPARS = $(COMMONPARS) $(ADDRELEASEPARS)
# 		parameters for debug build
DEBUGPARS = $(COMMONPARS) $(ADDDEBUGPARS)

#	Source files
SOURCES := $(call rwildcard,$(SRCDIR),*.$(SRCEXT))

#	List of objects created in debug build
DEBUGOBJECTS := $(subst $(SRCDIR)__, $(OBJDEBUGDIR)/,$(subst /,__,$(SOURCES:.$(SRCEXT)=.$(OBJEXT))))

#	List of objects created in release build
RELEASEOBJECTS := $(subst $(SRCDIR)__, $(OBJRELEASEDIR)/,$(subst /,__,$(SOURCES:.$(SRCEXT)=.$(OBJEXT))))

#	List of additional files for a binary package
PACKAGEFILES := $(wildcard $(PKGDIR)/*)

#	Binary package files in a destination directory
PKGBINFILES := $(PKGPREPDIR)/$(EXENAME) $(subst $(PKGDIR)/,$(PKGPREPDIR)/,$(PACKAGEFILES))

.SILENT:

# Phony Targets
.PHONY: release			# Release build - generate executable $BINDIR/$RELEASE/$EXENAME
.PHONY: debug			# Debug build - generate executable $BINDIR/$DEBUG/$EXENAME
.PHONY: prepare			# Prepare dependencies for build (for Steam Deck, see DEPENDENCIES above)
.PHONY: preparepkg		# Prepare binary package files (copy release executable and files from $PKGDIR into $PKGBINDIR/$PKGNAME)
.PHONY: createpkg		# Create zipped binary package (zip prepared binary package files into $PKGBINDIR/$PKGNAME.zip)
.PHONY: clean			# Clean binaries and objects (both release and debug build, inside $BINDIR and $OBJDIR)
.PHONY: dbgclean		# Clean binaries and objects from debug build (inside $BINDIR/$DEBUG and $OBJDIR/$DEBUG)
.PHONY: relclean		# Clean binaries and objects from release build (inside $BINDIR/$RELEASE and $OBJDIR/$RELEASE)
.PHONY: pkgclean		# Clean binary package artifacts (prepared files and zipped package)
.PHONY: pkgbinclean		# Clean zipped binary package
.PHONY: pkgprepclean	# Clean files prepared for binary package
.PHONY: cleanall		# Clean all artifacts (deletes $BINDIR, $OBJDIR, $PKGBINDIR)
.PHONY: install			# Run install script in prepared binary package files

# Prepare

prepare: 		$(DEPENDCHECKFILES)

$(DEPENDCHECKFILES) &:
	@echo "Prepare dependencies"
	deps=( $(DEPENDENCIES) ) && files=( $(DEPENDCHECKFILES) ) && ./$(PREPARESCRIPT) "$${#deps[@]}" "$${deps[@]}" "$${#files[@]}" "$${files[@]}"

# Build
# See also second expansion at the end

release: 			$(RELEASEPATH)

$(RELEASEPATH): $(DEPENDCHECKFILES) $(RELEASEOBJECTS) | $(RELEASEDIR)
	@echo "Linking into $@"
	$(CC) $(filter %.o,$^) $(RELEASEPARS) $(ADDLIBS) -o $@

debug: 				$(DEBUGPATH)

$(DEBUGPATH): $(DEPENDCHECKFILES) $(DEBUGOBJECTS) | $(DEBUGDIR)
	@echo "Linking into $@"
	$(CC) $(filter %.o,$^) $(DEBUGPARS) $(ADDLIBS) -o $@

# Binary package

preparepkg: 		$(PKGBINFILES)

$(PKGBINFILES) &: 	$(RELEASEPATH) $(PACKAGEFILES) | $(PKGBINDIR) $(PKGPREPDIR)
	@echo "Preparing binary package files in $(PKGPREPDIR)"
	rm -rf $(PKGPREPDIR)/*
	cp $(RELEASEPATH) $(PKGPREPDIR)/
	cp $(PKGDIR)/* $(PKGPREPDIR)/

createpkg: 			$(PKGBINPATH)

$(PKGBINPATH): 		$(PKGBINFILES)
	@echo "Zipping package files into a binary package $@"
	rm -f $(PKGBINPATH)
	cd $(PKGBINDIR) && zip -r $(PKGBIN) $(PKGNAME)

# Clean

clean: 	dbgclean relclean

relclean:
	@echo "Removing release build and objects"
	rm -f $(OBJRELEASEDIR)/*.$(OBJEXT)
	rm -f $(RELEASEPATH)
	
dbgclean:
	@echo "Removing debug build and objects"
	rm -f $(OBJDEBUGDIR)/*.$(OBJEXT)
	rm -f $(DEBUGPATH)

pkgclean: pkgprepclean pkgbinclean

pkgprepclean:
	@echo "Removing package files"
	rm -rf $(PKGPREPDIR)

pkgbinclean:
	@echo "Removing binary package"
	rm -f $(PKGBINPATH)

cleanall: clean pkgclean
	@echo "Removing object,binary and binary package directories"
	rm -rf $(PKGBINDIR) $(OBJDIR) $(BINDIR)

# Install

install: $(PKGBINFILES)
	@echo "Installing"
	cd $(PKGPREPDIR) && ./$(INSTALLSCRIPT)

# Directories

$(OBJRELEASEDIR) $(OBJDEBUGDIR): | $(OBJDIR)
	@echo "Creating directory $@"
	mkdir $@

$(RELEASEDIR) $(DEBUGDIR): | $(BINDIR)
	@echo "Creating directory $@"
	mkdir $@

$(PKGPREPDIR): | $(PKGBINDIR)
	@echo "Creating directory $@"
	mkdir $@

$(OBJDIR) $(BINDIR) $(PKGDIR) $(PKGBINDIR):
	@echo "Creating directory $@"
	mkdir $@

# Build - second expansion

.SECONDEXPANSION:

# 	Auxiliary function that uses compiler to generate list of headers the source file depends on
getheaders=$(shell $(CC) -M -I $(HEADERDIR)/ $(subst __,/,$(subst .$(OBJEXT),.$(SRCEXT),$(subst $(OBJRELEASEDIR)/,$(SRCDIR)/,$1))) 2>/dev/null | sed 's/.*\.o://' | sed 's/ \\//g' | tr '\n' ' ')

#	Release
#	Build object files

$(RELEASEOBJECTS): $(OBJRELEASEDIR)/%.$(OBJEXT): $$(subst __,/,$(SRCDIR)/%.$(SRCEXT)) $$(call getheaders,$$@) $$(DEPENDCHECKFILES) | $(OBJRELEASEDIR)
	@echo "Building $< into $@"
	$(CC) $< -c $(RELEASEPARS) -o $@

#	Debug
#	Build object files

$(DEBUGOBJECTS): $(OBJDEBUGDIR)/%.$(OBJEXT): $$(subst __,/,$(SRCDIR)/%.$(SRCEXT)) $$(call getheaders,$$@) $$(DEPENDCHECKFILES) | $(OBJDEBUGDIR)
	@echo "Building $< into $@"
	$(CC) $< -c $(DEBUGPARS) -o $@