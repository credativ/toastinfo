CREATE FUNCTION pg_toastinfo("any")
	RETURNS cstring
	AS '$libdir/toastinfo'
	LANGUAGE C IMMUTABLE;

CREATE FUNCTION pg_toastpointer("any")
	RETURNS oid
	AS '$libdir/toastinfo'
	LANGUAGE C IMMUTABLE STRICT;
