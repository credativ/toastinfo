PostgreSQL toastinfo Extension
==============================

This PostgreSQL extension exposes the internal storage structure of variable-length
datatypes, called `varlena`.

The function `pg_toastinfo` describes the storage form of a datum:

 * `null` for NULLs
 * `ordinary` for non-varlena datatypes
 * `short inline varlena` for varlena values up to 126 bytes (1 byte header)
 * `long inline varlena, (un)compressed` for varlena values up to 1GiB (4 bytes header)
 * `toasted varlena, (un)compressed` for varlena values up to 1GiB stored in TOAST tables
 * compressed varlenas show the compression method (pglz, lz4) in PG14+

The function `pg_toastpointer` returns a varlena's `chunk_id` oid in the
corresponding TOAST table. It returns NULL on non-varlena input.

PostgreSQL versions 9.1 and later are supported.

Example
-------

```
CREATE EXTENSION toastinfo;

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

SELECT a, length(b), pg_column_size(b), pg_toastinfo(b), pg_toastpointer(b) FROM t;
        a         │ length  │ pg_column_size │              pg_toastinfo              │ pg_toastpointer
──────────────────┼─────────┼────────────────┼────────────────────────────────────────┼─────────────────
 null             │       ∅ │              ∅ │ null                                   │               ∅
 default          │       7 │              8 │ short inline varlena                   │               ∅
 external-10      │       8 │              9 │ short inline varlena                   │               ∅
 external-200     │     200 │            204 │ long inline varlena, uncompressed      │               ∅
 external-10000   │   10000 │          10000 │ toasted varlena, uncompressed          │           16427
 external-1000000 │ 1000000 │        1000000 │ toasted varlena, uncompressed          │           16428
 extended-10      │       8 │              9 │ short inline varlena                   │               ∅
 extended-200     │     200 │            204 │ long inline varlena, uncompressed      │               ∅
 extended-10000   │   10000 │            125 │ long inline varlena, compressed (pglz) │               ∅
 extended-1000000 │ 1000000 │          11452 │ toasted varlena, compressed (pglz)     │           16429
 extended-10000   │   10000 │             58 │ long inline varlena, compressed (lz4)  │               ∅
 extended-1000000 │ 1000000 │           3936 │ toasted varlena, compressed (lz4)      │           16430
(12 rows)
```

License
-------
Author: Christoph Berg <cb@df7cb.de>

Copyright (c) 1996-2023, The PostgreSQL Global Development Group

Permission to use, copy, modify, and distribute this software and its documentation for any purpose, without fee, and without a written agreement is hereby granted, provided that the above copyright notice and this paragraph and the following two paragraphs appear in all copies.

IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATIONS TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

