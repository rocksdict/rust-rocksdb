// Copyright 2020 Tyler Neely
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

mod util;

use std::collections::HashMap;

use pretty_assertions::assert_eq;

use rocksdb::{Iterable as _, Options, ReadOptions, WriteBatch, WriteBatchIterator, DB};

use util::DBPath;

#[test]
fn test_write_batch_clear() {
    let mut batch = WriteBatch::default();
    batch.put(b"1", b"2");
    assert_eq!(batch.len(), 1);
    batch.clear();
    assert_eq!(batch.len(), 0);
    assert!(batch.is_empty());
}

#[test]
fn test_write_batch_wide_columns() {
    let path = DBPath::new("_rust_rocksdb_write_batch_widecolumns_test");
    {
        #[cfg(feature = "multi-threaded-cf")]
        let db = DB::open_default(&path).unwrap();
        #[cfg(not(feature = "multi-threaded-cf"))]
        let mut db = DB::open_default(&path).unwrap();
        db.create_cf("test", &Options::default()).unwrap();
        let cf = db.cf_handle("test").unwrap();

        let names = &[&b"567"[..], &b"1234"[..]];
        let values = &[&b"123f4"[..], &b"43d2100"[..]];

        let mut batch = WriteBatch::default();
        batch
            .put_entity_cf_opt(&cf, b"v1111", names, values)
            .unwrap();
        batch
            .put_entity_cf_opt(&cf, b"v2222", names, values)
            .unwrap();
        db.write(batch).unwrap();

        let ropt = ReadOptions::default();
        let columns = db.get_entity_cf_opt(&cf, b"v1111", &ropt).unwrap().unwrap();
        let columns = columns.iter().collect::<Vec<_>>();
        assert_eq!(columns.len(), 2);
        assert_eq!(columns[0].name, names[1]);
        assert_eq!(columns[0].value, values[1]);
        assert_eq!(columns[1].name, names[0]);
        assert_eq!(columns[1].value, values[0]);

        let columns = db.get_entity_cf_opt(&cf, b"v2222", &ropt).unwrap().unwrap();
        let columns = columns.iter().collect::<Vec<_>>();
        assert_eq!(columns.len(), 2);
        assert_eq!(columns[0].name, names[1]);
        assert_eq!(columns[0].value, values[1]);
        assert_eq!(columns[1].name, names[0]);
        assert_eq!(columns[1].value, values[0]);
    }
}

#[test]
fn test_write_batch_with_serialized_data() {
    struct Iterator {
        data: HashMap<Vec<u8>, Vec<u8>>,
    }

    impl WriteBatchIterator for Iterator {
        fn put(&mut self, key: Box<[u8]>, value: Box<[u8]>) {
            match self.data.remove(key.as_ref()) {
                Some(expect) => {
                    assert_eq!(value.as_ref(), expect.as_slice());
                }
                None => {
                    panic!("key not exists");
                }
            }
        }

        fn delete(&mut self, _: Box<[u8]>) {
            panic!("invalid delete operation");
        }
    }

    let mut kvs: HashMap<Vec<u8>, Vec<u8>> = HashMap::default();
    kvs.insert(vec![1], vec![2]);
    kvs.insert(vec![2], vec![3]);
    kvs.insert(vec![1, 2, 3, 4, 5], vec![4]);

    let mut b1 = WriteBatch::default();
    for (k, v) in &kvs {
        b1.put(k, v);
    }
    let data = b1.data();

    let b2 = WriteBatch::from_data(data);
    let mut it = Iterator { data: kvs };
    b2.iterate(&mut it);
}
