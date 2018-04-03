MODULES = toastinfo
EXTENSION = toastinfo
DATA = toastinfo--1.sql
REGRESS = toastinfo

ifndef PG_CONFIG
PG_CONFIG = pg_config
endif

PGXS = $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
