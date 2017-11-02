MODULES = toastinfo
EXTENSION = toastinfo
DATA = toastinfo--1.sql
REGRESS = toastinfo

PG_CONFIG = pg_config
PGXS = $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
