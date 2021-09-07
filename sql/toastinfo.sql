-- toastinfo.out: PG <= 13
-- toastinfo_1.out: PG14+

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
INSERT INTO t VALUES ('external-10', 'external'); -- short inline varlena
INSERT INTO t VALUES ('external-200', repeat('x', 200)); -- long inline varlena, uncompressed
INSERT INTO t VALUES ('external-10000', repeat('x', 10000)); -- toasted varlena, uncompressed
INSERT INTO t VALUES ('external-1000000', repeat('x', 1000000)); -- toasted varlena, uncompressed

ALTER TABLE t ALTER COLUMN b SET STORAGE EXTENDED;
INSERT INTO t VALUES ('extended-10', 'extended'); -- short inline varlena
INSERT INTO t VALUES ('extended-200', repeat('x', 200)); -- long inline varlena, uncompressed
INSERT INTO t VALUES ('extended-10000', repeat('x', 10000)); -- long inline varlena, compressed (pglz)
INSERT INTO t VALUES ('extended-1000000', repeat('x', 1000000)); -- toasted varlena, compressed (pglz)

ALTER TABLE t ALTER COLUMN b SET COMPRESSION lz4;
INSERT INTO t VALUES ('extended-10000', repeat('x', 10000)); -- long inline varlena, compressed (lz4)
INSERT INTO t VALUES ('extended-1000000', repeat('x', 1000000)); -- toasted varlena, compressed (lz4)

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

-- manually retrieve the toast content for some datums
SELECT a, substr(chunk_data, 1, 10) FROM t JOIN t_toast ON pg_toastpointer(t.b) = t_toast.chunk_id WHERE a ~ 'external' AND chunk_seq = 1 ORDER BY a;
