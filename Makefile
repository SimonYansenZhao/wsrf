PKGNAME  = wsrf
GITVER   = $(shell git rev-list --count HEAD)
VERSION  = 1.6.$(shell expr $(GITVER) - 36)
DATE     = $(shell date +%Y-%m-%d)
LPATH    = $(shell Rscript -e "write(Sys.getenv('R_LIBS_USER'), '')")
PKGSRC   = $(shell pwd)
PKGBASE  = $(shell basename $(PKGSRC))

REPOSITORY = repository/
FROMNODE = 31
TONODE   = 40

.PRECIOUS: weather.pdf

RNW=	weather.Rnw \
	fbis.Rnw

help:
	@echo "Manage the $(PKGNAME) R package\n\
	============================\n\n\
	check\tCheck for issues with the packaging\n\n\
	rebuild\tBuild and install the package.\n\
	build\tGenerate $(PKGNAME)_$(VERSION).tar.gz\n\n\
	install\t\t\tInstall with C++11 enabled\n\
	zip\tCreate a binary zip package\n\n\
	dist\tUpdate files on /var/www/\n\
	  src    \tCreate src packages (zip and tar).\n\
	  access \tCopy src to /var/www/access.\n\
	  repo   \tUpdate the repository at rattle.togaware.com.\n\n\
	"

######################################################################
# R Package Management

.PHONY: check
check: clean build
	R CMD check --as-cran --check-subdirs=yes $(PKGNAME)_$(VERSION).tar.gz

.PHONY: before_install
before_install:
	sed -i 's/^Version: .*/Version: $(VERSION)/' DESCRIPTION
	sed -i 's/^Date: .*/Date: $(DATE)/' DESCRIPTION
	mv package/* ./
	rm -rf package .travis.yml

.PHONY: build
build: clean
	sed -i 's/^Version: .*/Version: $(VERSION)/' DESCRIPTION && \
	sed -i 's/^Date: .*/Date: $(DATE)/' DESCRIPTION && \
	R CMD build $(PKGSRC)

.PHONY: install
install: clean build
	mkdir -p $(LPATH)
	R CMD INSTALL -l $(LPATH) --configure-args='--enable-c11=yes' $(PKGNAME)_$(VERSION).tar.gz

.PHONY: zip
zip: install
	(cd $(LPATH); zip -r9 - $(PKGNAME)) \
	>| $(PKGNAME)_$(VERSION).zip

.PHONY: rebuild
rebuild: install


##  install wsrf on multiple machines simultaneously.
##  Need to modify variable FROMNODE and TONODE if use different machines

.PHONY: handout
handout: build
	(for i in `seq $(FROMNODE) $(TONODE)`; \
        do \
            (echo ----------------- node$$i ------------------; \
            scp $(PKGNAME)_$(VERSION).tar.gz node$$i:; \
            ssh node$$i mkdir -p $(LPATH); \
            ssh node$$i R CMD INSTALL -l $(LPATH) --configure-args='--enable-c11=yes' $(PKGNAME)_$(VERSION).tar.gz; \
            ssh node$$i rm $(PKGNAME)_$(VERSION).tar.gz) > node$$i.log 2>&1 & \
        done; \
        wait)

########################################################################
# Source code management

.PHONY: src
src: realclean
	zip -r $(PKGNAME)_$(VERSION)_src.zip ./ -x $(PKGBASE)/*/*~;\
	tar zcvf $(PKGNAME)_$(VERSION)_src.tar.gz  --exclude="*~" --exclude="$(PKGNAME)_$(VERSION)_*" .

.PHONY: repo
repo: build
	cp $(PKGNAME)_$(VERSION).tar.gz $(REPOSITORY)
	-R --no-save < $(REPOSITORY)repository.R
	chmod go+r $(REPOSITORY)*
	lftp -f .lftp

.PHONY: access
access: src build
	install -m 0644 $(PKGNAME)_$(VERSION)_src.zip $(PKGNAME)_$(VERSION)_src.tar.gz /var/www/access/
	install -m 0644 $(PKGNAME)_$(VERSION).tar.gz /var/www/access/

.PHONY: dist
dist: src access repo

########################################################################
# manual

.PHONY: manual
manual:
	if [ -f $(PKGNAME).pdf ]; then rm $(PKGNAME).pdf; fi
	R CMD Rd2pdf $(PKGSRC)


########################################################################
# Misc

.PHONY: clean
clean:
	@rm -vf */*~ *~
	@rm -vf */.Rhistory
	@rm -rf $(PKGNAME).Rcheck rattle.Rcheck
	@rm -vf $(RNW:.Rnw=.aux) $(RNW:.Rnw=.out) $(RNW:.Rnw=.log)

.PHONY: realclean
realclean: clean
	rm -f weather_*.RData
ifeq ($(wildcard archive),) 	
	mkdir archive
endif
ifneq ($(wildcard $(PKGNAME)_*),)
	@mv -v $(wildcard $(PKGNAME)_*) archive/
endif
ifneq ($(wildcard *.pdf),)
	@echo hello $(wildcard *.pdf)
	@mv -v $(wildcard *.pdf) archive/
endif
ifneq ($(wildcard *.tex),)
	@mv -v $(wildcard *.tex) archive/
endif
