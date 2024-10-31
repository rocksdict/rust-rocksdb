#include "wide_columns_c.h"

#include <cstdlib>
#include <vector>

#include "rocksdb/rocksdb_namespace.h"
#include "rocksdb/wide_columns.h"

#include "rocksdb/compaction_filter.h"
#include "rocksdb/comparator.h"
#include "rocksdb/db.h"
#include "rocksdb/env.h"
#include "rocksdb/filter_policy.h"
#include "rocksdb/iterator.h"
#include "rocksdb/merge_operator.h"
#include "rocksdb/options.h"
#include "rocksdb/slice_transform.h"
#include "rocksdb/status.h"
#include "rocksdb/utilities/backup_engine.h"
#include "rocksdb/utilities/checkpoint.h"
#include "rocksdb/utilities/transaction.h"
#include "rocksdb/utilities/write_batch_with_index.h"
#include "rocksdb/write_batch.h"

using ROCKSDB_NAMESPACE::PinnableWideColumns;
using ROCKSDB_NAMESPACE::WideColumn;
using ROCKSDB_NAMESPACE::WideColumns;
using ROCKSDB_NAMESPACE::Slice;

using ROCKSDB_NAMESPACE::BackupID;
using ROCKSDB_NAMESPACE::ColumnFamilyHandle;
using ROCKSDB_NAMESPACE::DB;
using ROCKSDB_NAMESPACE::Iterator;
using ROCKSDB_NAMESPACE::ReadOptions;
using ROCKSDB_NAMESPACE::Status;
using ROCKSDB_NAMESPACE::WriteBatch;
using ROCKSDB_NAMESPACE::WriteOptions;

extern "C" {

struct rocksdb_t {
    DB* rep;
};
struct rocksdb_writebatch_t {
  WriteBatch rep;
};
struct rocksdb_readoptions_t {
  ReadOptions rep;
  // stack variables to set pointers to in ReadOptions
  Slice upper_bound;
  Slice lower_bound;
  Slice timestamp;
  Slice iter_start_ts;
};
struct rocksdb_writeoptions_t {
  WriteOptions rep;
};
struct rocksdb_column_family_handle_t {
  ColumnFamilyHandle* rep;
};
struct rocksdb_iterator_t {
  Iterator* rep;
};
struct rocksdb_pinnablewidecolumns_t {
    PinnableWideColumns rep;
};
struct rocksdb_widecolumns_t {
    WideColumns rep;
};

static bool SaveError(char** errptr, const Status& s) {
  assert(errptr != nullptr);
  if (s.ok()) {
    return false;
  } else if (*errptr == nullptr) {
    *errptr = strdup(s.ToString().c_str());
  } else {
    // TODO(sanjay): Merge with existing error?
    // This is a bug if *errptr is not created by malloc()
    free(*errptr);
    *errptr = strdup(s.ToString().c_str());
  }
  return true;
}

void rocksdb_put_entity_cf(rocksdb_t* db, const rocksdb_writeoptions_t* options,
                           rocksdb_column_family_handle_t* column_family,
                           const char* key, size_t keylen, size_t num_columns,
                           const char* const* names_list,
                           const size_t* names_list_sizes,
                           const char* const* values_list,
                           const size_t* values_list_sizes, char** errptr) {
  WideColumns columns;
  columns.reserve(num_columns);
  for (size_t i = 0; i < num_columns; i++) {
    WideColumn column(Slice(names_list[i], names_list_sizes[i]),
                      Slice(values_list[i], values_list_sizes[i]));
    columns.push_back(column);
  }
  SaveError(errptr, db->rep->PutEntity(options->rep, column_family->rep,
                                       Slice(key, keylen), columns));
}

rocksdb_pinnablewidecolumns_t* rocksdb_get_entity_cf(
    rocksdb_t* db, const rocksdb_readoptions_t* options,
    rocksdb_column_family_handle_t* column_family, const char* key,
    size_t keylen, char** errptr) {
  rocksdb_pinnablewidecolumns_t* columns = new (rocksdb_pinnablewidecolumns_t);
  SaveError(errptr, db->rep->GetEntity(options->rep, column_family->rep,
                                       Slice(key, keylen), &columns->rep));
  return columns;
}

rocksdb_widecolumns_t* rocksdb_iter_columns(const rocksdb_iterator_t* iter) {
  rocksdb_widecolumns_t* cols = new (rocksdb_widecolumns_t);
  cols->rep = std::move(iter->rep->columns());
  return cols;
}

void rocksdb_writebatch_put_entity_cf(
    rocksdb_writebatch_t* b, rocksdb_column_family_handle_t* column_family,
    const char* key, size_t keylen, size_t num_columns,
    const char* const* names_list, const size_t* names_list_sizes,
    const char* const* values_list, const size_t* values_list_sizes,
    char** errptr) {
  WideColumns columns;
  columns.reserve(num_columns);
  for (size_t i = 0; i < num_columns; i++) {
    WideColumn column(Slice(names_list[i], names_list_sizes[i]),
                      Slice(values_list[i], values_list_sizes[i]));

    columns.push_back(column);
  }
  SaveError(errptr,
            b->rep.PutEntity(column_family->rep, Slice(key, keylen), columns));
}

void rocksdb_pinnablewidecolumns_destroy(rocksdb_pinnablewidecolumns_t* v) {
  delete v;
}

size_t rocksdb_pinnablewidecolumns_size(
    const rocksdb_pinnablewidecolumns_t* v) {
  if (!v) {
    return 0;
  }
  return v->rep.columns().size();
}

const char* rocksdb_pinnablewidecolumns_name(
    const rocksdb_pinnablewidecolumns_t* v, const size_t n, size_t* name_len) {
  if (!v) {
    *name_len = 0;
    return nullptr;
  }
  Slice column = v->rep.columns()[n].name();
  *name_len = column.size();
  return column.data();
}

const char* rocksdb_pinnablewidecolumns_value(
    const rocksdb_pinnablewidecolumns_t* v, const size_t n, size_t* value_len) {
  if (!v) {
    *value_len = 0;
    return nullptr;
  }
  Slice column = v->rep.columns()[n].value();
  *value_len = column.size();
  return column.data();
}

void rocksdb_widecolumns_destroy(rocksdb_widecolumns_t* v) { delete v; }

size_t rocksdb_widecolumns_size(const rocksdb_widecolumns_t* v) {
  if (!v) {
    return 0;
  }
  return v->rep.size();
}

const char* rocksdb_widecolumns_name(const rocksdb_widecolumns_t* v,
                                     const size_t n, size_t* name_len) {
  if (!v) {
    *name_len = 0;
    return nullptr;
  }
  Slice column = v->rep[n].name();
  *name_len = column.size();
  return column.data();
}

const char* rocksdb_widecolumns_value(const rocksdb_widecolumns_t* v,
                                      const size_t n, size_t* value_len) {
  if (!v) {
    *value_len = 0;
    return nullptr;
  }
  Slice column = v->rep[n].value();
  *value_len = column.size();
  return column.data();
}

}  // end extern "C"
