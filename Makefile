APP=wsrf
GITVER=$(shell git rev-list --count HEAD)
VER=1.5.$(shell expr $(GITVER) + 23)
DATE=$(shell date +%Y-%m-%d)
LPATH=$(shell Rscript -e "write(Sys.getenv('R_LIBS_USER'), '')")
FROMNODE=31
TONODE=40
REPOSITORY=repository/
BOOST_INC=/usr/include
BOOST_LIB=/usr/lib
PKGSRC=$(shell basename `pwd`)

.PRECIOUS: weather.pdf

RNW=	weather.Rnw \
	fbis.Rnw

help:
	@echo "Manage the $(APP) R package\n\
	============================\n\n\
	check\tCheck for issues with the packaging\n\n\
	rebuild\tBuild and install the package.\n\
	build\tGenerate $(APP)_$(VER).tar.gz\n\n\
	install\t\t\tInstall with C++11 enabled\n\
	installboost\t\tInstall with boost enabled\n\
	installwithoutc11\tInstall on the local machine without parallel functionality\n\n\
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
	R CMD check --as-cran --check-subdirs=yes $(APP)_$(VER).tar.gz

.PHONY: before_install
before_install:
	sed -i 's/^Version: .*/Version: $(VER)/' DESCRIPTION
	sed -i 's/^Date: .*/Date: $(DATE)/' DESCRIPTION
	mv package/* ./
	rm -rf package .travis.yml

.PHONY: build
build:
	sed -i 's/^Version: .*/Version: $(VER)/' DESCRIPTION && \
	sed -i 's/^Date: .*/Date: $(DATE)/' DESCRIPTION && \
	cd ..;\
	R CMD build $(PKGSRC)

.PHONY: install
install: build
	mkdir -p $(LPATH)
	R CMD INSTALL -l $(LPATH) --configure-args='--enable-c11=yes' $(APP)_$(VER).tar.gz

.PHONY: installboost
installboost: build
	mkdir -p $(LPATH)
	R CMD INSTALL -l $(LPATH) --configure-args='--with-boost-include=$(BOOST_INC) --with-boost-lib=$(BOOST_LIB)' $(APP)_$(VER).tar.gz

.PHONY: installwithoutc11
installwithoutc11: build
	mkdir -p $(LPATH)
	R CMD INSTALL -l $(LPATH) --configure-args='--enable-c11=no' $(APP)_$(VER).tar.gz

.PHONY: zip
zip: install
	(cd /home/gjw/R/i686-pc-linux-gnu-library/2.15; zip -r9 - wsrf) \
	>| $(APP)_$(VER).zip

.PHONY: rebuild
rebuild: install


##  install wsrf on multiple machines simultaneously.
##  Need to modify variable FROMNODE and TONODE if use different machines

.PHONY: handout
handout: build
	(for i in `seq $(FROMNODE) $(TONODE)`; \
        do \
            (echo ----------------- node$$i ------------------; \
            scp $(APP)_$(VER).tar.gz node$$i:; \
            ssh node$$i mkdir -p $(LPATH); \
            ssh node$$i R CMD INSTALL -l $(LPATH) --configure-args='--enable-c11=yes' $(APP)_$(VER).tar.gz; \
            ssh node$$i rm $(APP)_$(VER).tar.gz) > node$$i.log 2>&1 & \
        done; \
        wait)


.PHONY: handoutboost
handoutboost: build
	(for i in `seq $(FROMNODE) $(TONODE)`; \
        do \
            (echo ----------------- node$$i ------------------; \
            scp $(APP)_$(VER).tar.gz node$$i:; \
            ssh node$$i mkdir -p $(LPATH); \
            ssh node$$i "R CMD INSTALL -l $(LPATH) --configure-args='--with-boost-include=$(BOOST_INC) --with-boost-lib=$(BOOST_LIB)' $(APP)_$(VER).tar.gz"; \
            ssh node$$i rm $(APP)_$(VER).tar.gz) > node$$i.log 2>&1 & \
        done; \
        wait)


.PHONY: handoutwithoutc11
handoutwithoutc11: build
	(for i in `seq $(FROMNODE) $(TONODE)`; \
        do \
            (echo ----------------- node$$i ------------------; \
            scp $(APP)_$(VER).tar.gz node$$i:; \
            ssh node$$i mkdir -p $(LPATH); \
            ssh node$$i "R CMD INSTALL -l $(LPATH) --configure-args='--enable-c11=no' $(APP)_$(VER).tar.gz"; \
            ssh node$$i rm $(APP)_$(VER).tar.gz) > node$$i.log 2>&1 & \
        done; \
        wait)
########################################################################
# Source code management

.PHONY: src
src:
	cd ..;\
	zip -r $(APP)_$(VER)_src.zip $(PKGSRC) -x $(PKGSRC)/*/*~;\
	tar zcvf $(APP)_$(VER)_src.tar.gz $(PKGSRC) --exclude="*~"

.PHONY: repo
repo: build
	cp $(APP)_$(VER).tar.gz $(REPOSITORY)
	-R --no-save < $(REPOSITORY)repository.R
	chmod go+r $(REPOSITORY)*
	lftp -f .lftp

.PHONY: access
access: src build
	install -m 0644 $(APP)_$(VER)_src.zip $(APP)_$(VER)_src.tar.gz /var/www/access/
	install -m 0644 $(APP)_$(VER).tar.gz /var/www/access/

.PHONY: dist
dist: src access repo

########################################################################
# manual

.PHONY: manual
manual:
	if [ -f $(APP).pdf ]; then rm $(APP).pdf; fi
	cd ..;\
	R CMD Rd2pdf $(PKGSRC)


########################################################################
# Misc

.PHONY: clean
clean:
	@rm -vf */*~ *~
	@rm -vf */.Rhistory
	@rm -rf $(APP).Rcheck rattle.Rcheck
	@rm -vf $(RNW:.Rnw=.aux) $(RNW:.Rnw=.out) $(RNW:.Rnw=.log)

.PHONY: realclean
realclean: clean
	rm -f weather_*.RData
ifeq ($(wildcard archive),) 	
	mkdir archive
endif
ifneq ($(wildcard $(APP)_*),)
	@mv -v $(wildcard $(APP)_*) archive/
endif
ifneq ($(wildcard *.pdf),)
	@mv -v $(wildcard *.pdf) archive/
endif
ifneq ($(wildcard *.tex),)
	@mv -v $(wildcard *.tex) archive/
endif
