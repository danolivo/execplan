# contrib/repeater/Makefile

MODULE_big = repeater
EXTENSION = repeater
EXTVERSION = 0.1
PGFILEDESC = "Repeater"
MODULES = repeater
OBJS = repeater.o $(WIN32RES)

fdw_srcdir = $(top_srcdir)/contrib/postgres_fdw/

PG_CPPFLAGS = -I$(libpq_srcdir) -I$(fdw_srcdir) -L$(fdw_srcdir)
SHLIB_LINK_INTERNAL = $(libpq)

DATA_built = $(EXTENSION)--$(EXTVERSION).sql

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
EXTRA_INSTALL = contrib/postgres_fdw
SHLIB_PREREQS = submake-libpq
subdir = contrib/repeater
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
#include $(top_builddir)/contrib/postgres_fdw/Makefile
include $(top_srcdir)/contrib/contrib-global.mk
endif

$(EXTENSION)--$(EXTVERSION).sql: init.sql
	cat $^ > $@
