#pragma once

#ifdef _WIN32
#ifdef ROCKSDB_DLL
#ifdef ROCKSDB_LIBRARY_EXPORTS
#define ROCKSDB_LIBRARY_API __declspec(dllexport)
#else
#define ROCKSDB_LIBRARY_API __declspec(dllimport)
#endif
#else
#define ROCKSDB_LIBRARY_API
#endif
#else
#define ROCKSDB_LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

#include "rocksdb/include/rocksdb/c.h"

/* Exported types */

typedef struct rocksdb_pinnablewidecolumns_t rocksdb_pinnablewidecolumns_t;
typedef struct rocksdb_widecolumns_t rocksdb_widecolumns_t;

/* DB operations */

extern ROCKSDB_LIBRARY_API void rocksdb_put_entity_cf(
    rocksdb_t* db, const rocksdb_writeoptions_t* options,
    rocksdb_column_family_handle_t* column_family, const char* key,
    size_t keylen, size_t num_columns, const char* const* names_list,
    const size_t* names_list_sizes, const char* const* values_list,
    const size_t* values_list_sizes, char** errptr);

extern ROCKSDB_LIBRARY_API rocksdb_pinnablewidecolumns_t* rocksdb_get_entity_cf(
    rocksdb_t* db, const rocksdb_readoptions_t* options,
    rocksdb_column_family_handle_t* column_family, const char* key,
    size_t keylen, char** errptr);

extern ROCKSDB_LIBRARY_API rocksdb_widecolumns_t* rocksdb_iter_columns(
    const rocksdb_iterator_t* iter);

extern ROCKSDB_LIBRARY_API void rocksdb_writebatch_put_entity_cf(
    rocksdb_writebatch_t* b, rocksdb_column_family_handle_t* column_family,
    const char* key, size_t keylen, size_t num_columns,
    const char* const* names_list, const size_t* names_list_sizes,
    const char* const* values_list, const size_t* values_list_sizes,
    char** errptr);

extern ROCKSDB_LIBRARY_API void rocksdb_pinnablewidecolumns_destroy(
    rocksdb_pinnablewidecolumns_t* v);
extern ROCKSDB_LIBRARY_API size_t
rocksdb_pinnablewidecolumns_size(const rocksdb_pinnablewidecolumns_t* v);
extern ROCKSDB_LIBRARY_API const char* rocksdb_pinnablewidecolumns_name(
    const rocksdb_pinnablewidecolumns_t* v, const size_t n, size_t* name_len);
extern ROCKSDB_LIBRARY_API const char* rocksdb_pinnablewidecolumns_value(
    const rocksdb_pinnablewidecolumns_t* v, const size_t n, size_t* value_len);
extern ROCKSDB_LIBRARY_API void rocksdb_widecolumns_destroy(
    rocksdb_widecolumns_t* v);
extern ROCKSDB_LIBRARY_API size_t
rocksdb_widecolumns_size(const rocksdb_widecolumns_t* v);
extern ROCKSDB_LIBRARY_API const char* rocksdb_widecolumns_name(
    const rocksdb_widecolumns_t* v, const size_t n, size_t* name_len);
extern ROCKSDB_LIBRARY_API const char* rocksdb_widecolumns_value(
    const rocksdb_widecolumns_t* v, const size_t n, size_t* value_len);

#ifdef __cplusplus
} /* end extern "C" */
#endif
