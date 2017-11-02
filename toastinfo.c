#include <postgres.h>
#include <fmgr.h>
#include <utils/lsyscache.h>
#include <access/tuptoaster.h>
#include <utils/expandeddatum.h>

PG_MODULE_MAGIC;

char *toast_datum_info(Datum value);

char *
toast_datum_info(Datum value)
{
	struct varlena *attr = (struct varlena *) DatumGetPointer(value);

	if (VARATT_IS_EXTERNAL_ONDISK(attr))
	{
		struct varatt_external toast_pointer;

		VARATT_EXTERNAL_GET_POINTER(toast_pointer, attr);

		if (toast_pointer.va_extsize < toast_pointer.va_rawsize - VARHDRSZ)
			return psprintf("toasted varlena, compressed");
		else
			return psprintf("toasted varlena, uncompressed");
	}
	else if (VARATT_IS_EXTERNAL_INDIRECT(attr))
	{
		struct varatt_indirect toast_pointer;

		VARATT_EXTERNAL_GET_POINTER(toast_pointer, attr);

		return psprintf("indirect in-memory varlena");
	}
	else if (VARATT_IS_EXTERNAL_EXPANDED(attr))
	{
		return psprintf("expanded in-memory varlena");
	}
	else if (VARATT_IS_SHORT(attr))
	{
		return psprintf("short inline varlena");
	}
	else
	{
		if (VARATT_IS_COMPRESSED(attr))
			return psprintf("long inline varlena, compressed");
		else
			return psprintf("long inline varlena, uncompressed");
	}
}

PG_FUNCTION_INFO_V1 (pg_toastinfo);

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

	if (!VARATT_IS_EXTERNAL_ONDISK(attr)) /* non-toasted varlena, return NULL */
		PG_RETURN_NULL();

	VARATT_EXTERNAL_GET_POINTER(toast_pointer, attr);

	PG_RETURN_OID(toast_pointer.va_valueid);
}

