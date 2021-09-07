/*
 Author: Christoph Berg <cb@df7cb.de>

 Copyright: Copyright (c) 1996-2020, The PostgreSQL Global Development Group

 Permission to use, copy, modify, and distribute this software and its
 documentation for any purpose, without fee, and without a written agreement
 is hereby granted, provided that the above copyright notice and this
 paragraph and the following two paragraphs appear in all copies.

 IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING
 LOST PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS
 DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA HAS BEEN ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.

 THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATIONS TO
 PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
*/

#include <postgres.h>
#include <fmgr.h>
#include <utils/lsyscache.h>
#if PG_VERSION_NUM >= 130000
#include <access/detoast.h>
#else
#include <access/tuptoaster.h>
#endif
#if PG_VERSION_NUM >= 90500
#include <utils/expandeddatum.h>
#endif

#if PG_VERSION_NUM < 90400
/*
 * Macro to fetch the possibly-unaligned contents of an EXTERNAL datum
 * into a local "struct varatt_external" toast pointer.  This should be
 * just a memcpy, but some versions of gcc seem to produce broken code
 * that assumes the datum contents are aligned.  Introducing an explicit
 * intermediate "varattrib_1b_e *" variable seems to fix it.
 */
#define VARATT_EXTERNAL_GET_POINTER(toast_pointer, attr) \
do { \
	varattrib_1b_e *attre = (varattrib_1b_e *) (attr); \
	Assert(VARATT_IS_EXTERNAL(attre)); \
	Assert(VARSIZE_EXTERNAL(attre) == sizeof(toast_pointer) + VARHDRSZ_EXTERNAL); \
	memcpy(&(toast_pointer), VARDATA_EXTERNAL(attre), sizeof(toast_pointer)); \
} while (0)
#endif

PG_MODULE_MAGIC;

char *toast_datum_info(Datum value);

char *
toast_datum_info(Datum value)
{
	struct varlena *attr = (struct varlena *) DatumGetPointer(value);

#if PG_VERSION_NUM >= 90400
	if (VARATT_IS_EXTERNAL_ONDISK(attr))
#else
	if (VARATT_IS_EXTERNAL(attr))
#endif
	{
		struct varatt_external toast_pointer;

		VARATT_EXTERNAL_GET_POINTER(toast_pointer, attr);

#if PG_VERSION_NUM >= 90400
		if (VARATT_EXTERNAL_IS_COMPRESSED(toast_pointer))
#else
		if (toast_pointer.va_extsize < toast_pointer.va_rawsize - VARHDRSZ)
#endif
			return "toasted varlena, compressed";
		else
			return "toasted varlena, uncompressed";
	}
#if PG_VERSION_NUM >= 90400
	else if (VARATT_IS_EXTERNAL_INDIRECT(attr))
	{
		struct varatt_indirect toast_pointer;

		VARATT_EXTERNAL_GET_POINTER(toast_pointer, attr);

		return "indirect in-memory varlena";
	}
#endif
#if PG_VERSION_NUM >= 90500
	else if (VARATT_IS_EXTERNAL_EXPANDED(attr))
	{
		return "expanded in-memory varlena";
	}
#endif
	else if (VARATT_IS_SHORT(attr))
	{
		return "short inline varlena";
	}
	else
	{
		if (VARATT_IS_COMPRESSED(attr))
			return "long inline varlena, compressed";
		else
			return "long inline varlena, uncompressed";
	}
}

PG_FUNCTION_INFO_V1 (pg_toastinfo);
Datum pg_toastinfo (PG_FUNCTION_ARGS);

Datum
pg_toastinfo (PG_FUNCTION_ARGS)
{
	Datum       value;
	char       *result;
	int         typlen;

	/* On first call, get the input type's typlen, and save at *fn_extra */
	if (fcinfo->flinfo->fn_extra == NULL)
	{
		/* Lookup the datatype of the supplied argument */
		Oid         argtypeid = get_fn_expr_argtype(fcinfo->flinfo, 0);

		typlen = get_typlen(argtypeid);
		if (typlen == 0)        /* should not happen */
			elog(ERROR, "cache lookup failed for type %u", argtypeid);

		fcinfo->flinfo->fn_extra = MemoryContextAlloc(fcinfo->flinfo->fn_mcxt,
				sizeof(int));
		*((int *) fcinfo->flinfo->fn_extra) = typlen;
	}
	else
		typlen = *((int *) fcinfo->flinfo->fn_extra);

	if (PG_ARGISNULL(0))
		result = "null";
	else
	{
		value = PG_GETARG_DATUM(0);
		if (typlen == -1)
		{
			/* varlena type, possibly toasted */
			result = toast_datum_info(value);
		}
		else if (typlen == -2)
		{
			/* cstring */
			result = "cstring";
		}
		else
		{
			/* ordinary fixed-width type */
			result = "ordinary";
		}
	}

	PG_RETURN_CSTRING(result);
}

PG_FUNCTION_INFO_V1 (pg_toastpointer);
Datum pg_toastpointer (PG_FUNCTION_ARGS);

Datum
pg_toastpointer (PG_FUNCTION_ARGS)
{
	Datum       value = PG_GETARG_DATUM(0);
	int         typlen;
	struct varlena *attr;
	struct varatt_external toast_pointer;

	/* On first call, get the input type's typlen, and save at *fn_extra */
	if (fcinfo->flinfo->fn_extra == NULL)
	{
		/* Lookup the datatype of the supplied argument */
		Oid         argtypeid = get_fn_expr_argtype(fcinfo->flinfo, 0);

		typlen = get_typlen(argtypeid);
		if (typlen == 0)        /* should not happen */
			elog(ERROR, "cache lookup failed for type %u", argtypeid);

		fcinfo->flinfo->fn_extra = MemoryContextAlloc(fcinfo->flinfo->fn_mcxt,
				sizeof(int));
		*((int *) fcinfo->flinfo->fn_extra) = typlen;
	}
	else
		typlen = *((int *) fcinfo->flinfo->fn_extra);

	if (typlen != -1) /* not a varlena, return NULL */
		PG_RETURN_NULL();

	attr = (struct varlena *) DatumGetPointer(value);

#if PG_VERSION_NUM >= 90400
	if (!VARATT_IS_EXTERNAL_ONDISK(attr)) /* non-toasted varlena, return NULL */
#else
	if (!VARATT_IS_EXTERNAL(attr)) /* non-toasted varlena, return NULL */
#endif
		PG_RETURN_NULL();

	VARATT_EXTERNAL_GET_POINTER(toast_pointer, attr);

	PG_RETURN_OID(toast_pointer.va_valueid);
}

