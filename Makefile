## Build a project and combine built executable into binary package together with additional files
## Restrictions: 
##		- one generated binary file (executable)
##		- single extension of compiled source files
##		- compilation with gcc or compatible

# Parallel by default

CPUS ?= $(shell nproc || echo 1)
JMUL := 4
JOBS := $(shell echo $$(($(CPUS) * $(JMUL))))
MAKEFLAGS += --jobs=$(JOBS)

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
#		name of symbolic link to release binary
SYMRELEASE = launch
#		name of symbolic link to debug binary
SYMDEBUG = launchdebug

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
ADDDEBUGPARS = -g
# 		Additional libraries parameters
ADDLIBS = -pthread -lncurses -lsystemd -lhidapi-hidraw

#	Install

# 		Install script name (has to be in $PKGDIR)
INSTALLSCRIPT = install.sh

#	Prepare

#		Prepare script file path, used to install dependencies with missing files in the system
PREPARESCRIPT = scripts/prepare.sh

#	Dependency check (Steam Deck)
#	Some libraries have to be reinstalled on Steam Deck because haeder files are missing

# 	Check files - these files existence will be checked to determine if 
#	the corresponding dependency is properly installed
DEPENDCHECKFILES := $(firstword $(wildcard /usr/include/c++/*/) /usr/include/c++/*/)vector /usr/include/errno.h /usr/include/linux/can/error.h /usr/include/ncurses.h /usr/include/systemd/sd-device.h /usr/include/hidapi/hidapi.h

# 	Dependencies - names of packages to install with pacman -S
#	if the corresponding check file is not found
DEPENDENCIES := gcc glibc linux-api-headers ncurses systemd-libs hidapi

# Functions

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

getiterator=$(shell seq 1 $(words $1))
getpos=$(firstword $(foreach i,$(call getiterator,$2),$(if $(findstring $1,$(word $i,$2)),$i )))
getmatch=$(word $(call getpos,$1,$2),$3)
remove=$(foreach x,$2,$(if $(findstring $1,$x,,$x )))
removefrom=$(foreach i,$(call getiterator,$2),$(if $(shell if [[ $i -lt $1 ]]; then echo "x"; fi),$(word $i,$2) ))

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


SILENTFLAG := $(if $(findstring s,$(word 1, $(MAKEFLAGS))),$(if $(findstring -,$(word 1, $(MAKEFLAGS))),,true))
INSILENT := $(if $(SILENTFLAG),&>/dev/null)
DUMMYSHELLSILENT := $(if $(SILENTFLAG),&>/dev/null,1>&2)
SHELLSILENT := $(if $(SILENTFLAG),2>/dev/null,)

# Comment this section if make should not be silent
.SILENT:
# Silent inside recipes loops,ifs etc:
INSILENT := &>/dev/null
DUMMYSHELLSILENT := &>/dev/null
SHELLSILENT := 2>/dev/null

$(shell echo "Makefile flags: $(MAKEFLAGS)" $(DUMMYSHELLSILENT))

$(shell echo "Makefile targets: $(MAKECMDGOALS)" $(DUMMYSHELLSILENT))

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
.PHONY: uninstall		# Uninstall package

.DEFAULT_GOAL := release

# Prepare

RUNDEPS := mk_deps_run.tmp
FINISHDEPS := mk_deps.tmp
CHECKDEPS := mk_chdeps.tmp

ifndef NOPREPARE

TEMPFILESNEC := $(or $(if $(MAKECMDGOALS),,x),$(findstring release,$(MAKECMDGOALS)),$(findstring debug,$(MAKECMDGOALS))\
,$(findstring install,$(MAKECMDGOALS)),$(findstring createpkg,$(MAKECMDGOALS)),$(findstring preparepkg,$(MAKECMDGOALS))\
,$(findstring prepare,$(MAKECMDGOALS)))

ifneq ($(TEMPFILESNEC),)
$(shell echo "Creating temporary files." 1>&2)
#	Temporary file for running double-colon rules only when target doesn't exist
$(shell touch -d "1990-01-01" $(RUNDEPS) &>/dev/null)
$(shell echo "touch -d "1990-01-01" $(RUNDEPS) &>/dev/null" $(DUMMYSHELLSILENT))

#	Temporary file for running finishing commands if dependencies are reinstalled
$(shell touch $(FINISHDEPS) &>/dev/null)
$(shell echo "touch $(FINISHDEPS) &>/dev/null" $(DUMMYSHELLSILENT))

#	Final file
$(shell touch -d "1990-01-01"  $(CHECKDEPS) &>/dev/null)
$(shell echo "touch -d "1990-01-01"  $(CHECKDEPS) &>/dev/null" $(DUMMYSHELLSILENT))
endif

#	Temporary file for storing information between preparation and finish script
MEMORYDEPS := mk_deps_mem.tmp

prepare: 		$(CHECKDEPS)

BASICFLAGS = $(if $(findstring -,$(word 1,$(MAKEFLAGS))),,-$(word 1,$(MAKEFLAGS)))

# Clean temporary files after dependencies are checked
$(CHECKDEPS): $(FINISHDEPS)
	@echo "Cleaning temporary files"
	rm -f $(FINISHDEPS)
	rm -f $(RUNDEPS)
	rm -f $(CHECKDEPS)
ifneq ($(MAKECMDGOALS),prepare)
	@if [[ -f $(MEMORYDEPS) && $(MEMORYDEPS) -nt Makefile ]]; then\
		echo "rm -f $(MEMORYDEPS)" $(INSILENT);\
		rm -f $(MEMORYDEPS);\
		echo "Dependencies were reinstalled. Restarting make.";\
		echo "$(MAKE) $(BASICFLAGS) $(MAKECMDGOALS)" $(INSILENT);\
		$(MAKE) $(BASICFLAGS) $(MAKECMDGOALS);\
		echo "Ignore error below. Make has finished.";\
		echo "false" $(INSILENT);\
		false;\
	fi
endif
	rm -f $(MEMORYDEPS)

#	Initially memory file does not exist - require it.
#	Creating it will trigger updates of dependencies (see below)
#	After that, run finishing commands
$(FINISHDEPS): $(MEMORYDEPS)
	@if grep -q true $(MEMORYDEPS); then \
		echo "Reenabling read-only filesystem";\
		echo "steamos-readonly enable &>/dev/null" $(INSILENT);\
		sudo steamos-readonly enable &>/dev/null;\
	fi
	@echo "Required dependencies reinstalled."

#	Memory file is created by preparation commands of dependency reinstalls
#	If no dependency needs a reinstall, file is created with old date
#	(to prevent finishing script from running)
$(MEMORYDEPS): $(DEPENDCHECKFILES)
	@if [ ! -f $(MEMORYDEPS) ]; then \
		touch -d "1990-01-01" $(MEMORYDEPS); \
		echo "touch -d "1990-01-01" $(MEMORYDEPS)" $(INSILENT);\
	fi

#	Preparation. Fake dependency with old date is added here to make
#	the double-colon recipe run only when any target from group does not exist
$(DEPENDCHECKFILES) &:: $(RUNDEPS)
	@echo "Some dependencies missing. Reinstall."
	@echo "Check read-only filesystem"
	@if sudo steamos-readonly status 2>/dev/null | grep -q 'enabled';then \
		echo "true">$(MEMORYDEPS);\
		echo "Read only filesystem enabled. Disabling...";\
		echo "steamos-readonly disable &>/dev/null" $(INSILENT);\
		sudo steamos-readonly disable &>/dev/null;\
	else \
		echo "false">$(MEMORYDEPS);\
	fi
	@echo "Initializing package manager..."
	sudo pacman-key --init &>/dev/null
	sudo pacman-key --populate &>/dev/null

else # ifndef NOPREPARE

prepare: ;

endif # ifndef NOPREPARE

#  See second expansion at the bottom

# Build

release: 			$(SYMRELEASE)
debug: 				$(SYMDEBUG)

$(SYMRELEASE):	$(RELEASEPATH)
$(SYMDEBUG):	$(DEBUGPATH)

$(SYMRELEASE) $(SYMDEBUG):
	@echo "Creating symbolic link for quick launch: ./$@"
	rm -f $@
	ln -s $^ $@

$(RELEASEPATH): $(RELEASEOBJECTS) | $(CHECKDEPS) $(RELEASEDIR)
	@echo "Linking into $@"
	$(CC) $(filter %.o,$^) $(RELEASEPARS) $(ADDLIBS) -o $@

$(DEBUGPATH): $(DEBUGOBJECTS) | $(CHECKDEPS) $(DEBUGDIR)
	@echo "Linking into $@"
	$(CC) $(filter %.o,$^) $(DEBUGPARS) $(ADDLIBS) -o $@

# See also second expansion at the end

# Binary package

preparepkg: 		$(PKGBINFILES)

$(PKGBINFILES) &: 	$(SYMRELEASE) $(PACKAGEFILES) | $(PKGBINDIR) $(PKGPREPDIR)
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

clean: 	dbgclean relclean tmpclean
	rm -f $(MKTMPFILE)

relclean:
	@echo "Removing release build and objects"
	rm -f $(OBJRELEASEDIR)/*.$(OBJEXT)
	rm -f $(RELEASEPATH)
	rm -f $(SYMRELEASE)
	
dbgclean:
	@echo "Removing debug build and objects"
	rm -f $(OBJDEBUGDIR)/*.$(OBJEXT)
	rm -f $(DEBUGPATH)
	rm -f $(SYMDEBUG)

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
	rm -f $(FINISHDEPS)
	rm -f $(RUNDEPS)
	rm -f $(CHECKDEPS)

tmpclean:
	@echo "Removing temporary files."
	rm -f $(FINISHDEPS)
	rm -f $(RUNDEPS)
	rm -f $(CHECKDEPS)

# Install

install: $(PKGBINFILES)
	@echo "Installing"
	cd $(PKGPREPDIR) && ./$(INSTALLSCRIPT)

# Uninstall

.IGNORE: uninstall

uninstall:
	@echo "Uninstalling"
	[ -f ~/$(EXENAME)/uninstall.sh ] && ~/$(EXENAME)/uninstall.sh || echo "Package is not installed."

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

# Prepare

ifndef NOPREPARE

.IGNORE: $(DEPENDCHECKFILES)

#	Reinstall. Normal multiple target. Each dependency check file is
#	checked if it is missing. If it is, then package is reinstalled.
#	Second expansion of order only dependency is so that this targets will not run in parallel
$(DEPENDCHECKFILES) :: $(RUNDEPS) | $$(call removefrom,$$(call getpos,$$@,$(DEPENDCHECKFILES)),$(DEPENDCHECKFILES))
	@echo -e "Missing $@. Reinstalling \e[1m$(call getmatch,$@,$(DEPENDCHECKFILES),$(DEPENDENCIES))\e[0m..."
	sudo pacman -S --noconfirm $(call getmatch,$@,$(DEPENDCHECKFILES),$(DEPENDENCIES)) &>/dev/null

endif # ifndef NOPREPARE

# Build

GETHEADERSNEC := $(or $(if $(MAKECMDGOALS),,x),$(findstring release,$(MAKECMDGOALS)),$(findstring debug,$(MAKECMDGOALS))\
,$(findstring install,$(MAKECMDGOALS)),$(findstring createpkg,$(MAKECMDGOALS)),$(findstring preparepkg,$(MAKECMDGOALS)))

ifneq ($(GETHEADERSNEC),)
# 	Auxiliary function that uses compiler to generate list of headers the source file depends on
getheaders=$(shell $(CC) -M -I $(HEADERDIR)/ $1 2>/dev/null | sed 's/.*\.o://' | sed 's/ \\//g' | tr '\n' ' '; \
			 echo "$(CC) -M -I $(HEADERDIR)/ $1 2>/dev/null | sed 's/.*\.o://' | sed 's/ \\//g' | tr '\n' ' '" $(DUMMYSHELLSILENT))
else
getheaders=
endif

#	Release
#	Build object files

$(RELEASEOBJECTS): $(OBJRELEASEDIR)/%.$(OBJEXT): $$(subst __,/,$(SRCDIR)/%.$(SRCEXT)) \
  $$(call getheaders,$$(subst __,/,$(SRCDIR)/%.$(SRCEXT))) | $$(CHECKDEPS) $(OBJRELEASEDIR)
	@echo "Building $< into $@"
	$(CC) $< -c $(RELEASEPARS) -o $@

#	Debug
#	Build object files

$(DEBUGOBJECTS): $(OBJDEBUGDIR)/%.$(OBJEXT): $$(subst __,/,$(SRCDIR)/%.$(SRCEXT)) \
  $$(call getheaders,$$(subst __,/,$(SRCDIR)/%.$(SRCEXT))) | $$(CHECKDEPS) $(OBJDEBUGDIR)
	@echo "Building $< into $@"
	$(CC) $< -c $(DEBUGPARS) -o $@