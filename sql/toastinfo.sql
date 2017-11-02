CREATE EXTENSION toastinfo;

CREATE TABLE x (
	int int,
	bigint bigint,
	float float,
	numeric numeric,
	text text,
	bytea bytea
);
INSERT INTO x VALUES(1, 1, 1, 1, '', '');
SELECT pg_toastinfo(int) AS int,
       pg_toastinfo(bigint) AS bigint,
       pg_toastinfo(float) AS float,
       pg_toastinfo(numeric) AS numeric,
       pg_toastinfo(text) AS text,
       pg_toastinfo(bytea) AS bytea
FROM x;
SELECT pg_toastpointer(int) AS int,
       pg_toastpointer(bigint) AS bigint,
       pg_toastpointer(float) AS float,
       pg_toastpointer(numeric) AS numeric,
       pg_toastpointer(text) AS text,
       pg_toastpointer(bytea) AS bytea
FROM x;

CREATE TABLE t (
	a text,
	b text
);

INSERT INTO t VALUES ('null', NULL);
INSERT INTO t VALUES ('default', 'default');

ALTER TABLE t ALTER COLUMN b SET STORAGE EXTERNAL;
INSERT INTO t VALUES ('external-10', 'external'); -- inline
INSERT INTO t VALUES ('external-10000', repeat('1',10000)); -- toast uncompressed
INSERT INTO t VALUES ('external-1000000', repeat('2',1000000)); -- toast uncompressed

ALTER TABLE t ALTER COLUMN b SET STORAGE EXTENDED;
INSERT INTO t VALUES ('extended-10', 'extended'); -- inline
INSERT INTO t VALUES ('extended-10000', repeat('3',10000)); -- inline compressed
INSERT INTO t VALUES ('extended-1000000', repeat('4',1000000)); -- toast compressed

-- call pg_toastinfo and pg_toastpointer twice in parallel to check the typlen caching
-- for pg_toastinfo, just show if an oid was returned or now
SELECT a, pg_toastinfo(a) a, pg_toastpointer(a) > 0 aptr,
          pg_toastinfo(b) b, pg_toastpointer(b) > 0 bptr FROM t;

-- give t's toast table a predictable name we can use below
DO $$
DECLARE
	o oid;
BEGIN
	SELECT oid INTO o FROM pg_class WHERE relname = 't';
	EXECUTE format('CREATE VIEW t_toast AS SELECT * FROM pg_toast.pg_toast_%s', o);
END;
$$ LANGUAGE plpgsql;

-- manually retrieve the toast content for some datum
\x
SELECT a, chunk_seq, chunk_data FROM t JOIN t_toast ON pg_toastpointer(t.b) = t_toast.chunk_id WHERE t.a = 'external-10000';
