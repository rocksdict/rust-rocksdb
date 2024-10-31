use crate::ffi;
use std::{marker::PhantomData, ptr::slice_from_raw_parts};

pub struct WideColumn<'a> {
    pub name: &'a [u8],
    pub value: &'a [u8],
}

pub struct WideColumns<'a> {
    inner: *const ffi::rocksdb_widecolumns_t,
    columns_size: usize,
    iter: PhantomData<&'a ()>,
}

impl<'a> WideColumns<'a> {
    pub unsafe fn from_c(inner: *const ffi::rocksdb_widecolumns_t) -> Self {
        Self {
            inner,
            columns_size: ffi::rocksdb_widecolumns_size(inner),
            iter: PhantomData,
        }
    }

    pub fn len(&self) -> usize {
        self.columns_size
    }

    pub fn is_empty(&self) -> bool {
        self.columns_size == 0
    }

    pub unsafe fn get_column_name_unchecked(&self, idx: usize) -> &[u8] {
        let mut name_len: usize = 0;
        let name = ffi::rocksdb_widecolumns_name(self.inner, idx, &mut name_len);
        &*slice_from_raw_parts(name as *const u8, name_len)
    }

    pub unsafe fn get_column_value_unchecked(&self, idx: usize) -> &[u8] {
        let mut value_len: usize = 0;
        let value = ffi::rocksdb_widecolumns_value(self.inner, idx, &mut value_len);
        &*slice_from_raw_parts(value as *const u8, value_len)
    }

    pub unsafe fn get_column_unchecked(&self, idx: usize) -> WideColumn {
        WideColumn {
            name: self.get_column_name_unchecked(idx),
            value: self.get_column_value_unchecked(idx),
        }
    }
}

pub struct PinnableWideColumns<'a> {
    inner: *const ffi::rocksdb_pinnablewidecolumns_t,
    columns_size: usize,
    iter: PhantomData<&'a ()>,
}

impl<'a> PinnableWideColumns<'a> {
    pub unsafe fn from_c(inner: *const ffi::rocksdb_pinnablewidecolumns_t) -> Self {
        Self {
            inner,
            columns_size: ffi::rocksdb_pinnablewidecolumns_size(inner),
            iter: PhantomData,
        }
    }

    pub fn len(&self) -> usize {
        self.columns_size
    }

    pub fn is_empty(&self) -> bool {
        self.columns_size == 0
    }

    pub unsafe fn get_column_name_unchecked(&self, idx: usize) -> &[u8] {
        let mut name_len: usize = 0;
        let name = ffi::rocksdb_pinnablewidecolumns_name(self.inner, idx, &mut name_len);
        &*slice_from_raw_parts(name as *const u8, name_len)
    }

    pub unsafe fn get_column_value_unchecked(&self, idx: usize) -> &[u8] {
        let mut value_len: usize = 0;
        let value = ffi::rocksdb_pinnablewidecolumns_value(self.inner, idx, &mut value_len);
        &*slice_from_raw_parts(value as *const u8, value_len)
    }

    pub unsafe fn get_column_unchecked(&self, idx: usize) -> WideColumn {
        WideColumn {
            name: self.get_column_name_unchecked(idx),
            value: self.get_column_value_unchecked(idx),
        }
    }
}

pub trait Iterable {
    type Item<'me>
    where
        Self: 'me;

    type Iter<'me>: Iterator<Item = Self::Item<'me>>
    where
        Self: 'me;

    fn iter(&self) -> Self::Iter<'_>;
}

pub struct PinnableWideColumnsIter<'a> {
    columns: &'a PinnableWideColumns<'a>,
    current_idx: usize,
}

impl<'me> Iterator for PinnableWideColumnsIter<'me> {
    type Item = WideColumn<'me>;

    fn next(&mut self) -> Option<Self::Item> {
        if self.current_idx < self.columns.len() {
            let column = unsafe { self.columns.get_column_unchecked(self.current_idx) };
            self.current_idx += 1;
            Some(WideColumn {
                name: column.name,
                value: column.value,
            })
        } else {
            None
        }
    }
}

impl<'a> Iterable for PinnableWideColumns<'a> {
    type Item<'me> = WideColumn<'me>
    where
        Self: 'me;

    type Iter<'me> = PinnableWideColumnsIter<'me>
    where
        Self: 'me;

    fn iter(&self) -> Self::Iter<'_> {
        PinnableWideColumnsIter {
            columns: self,
            current_idx: 0,
        }
    }
}

pub struct WideColumnsIter<'a> {
    columns: &'a WideColumns<'a>,
    current_idx: usize,
}

impl<'me> Iterator for WideColumnsIter<'me> {
    type Item = WideColumn<'me>;

    fn next(&mut self) -> Option<Self::Item> {
        if self.current_idx < self.columns.len() {
            let column = unsafe { self.columns.get_column_unchecked(self.current_idx) };
            self.current_idx += 1;
            Some(WideColumn {
                name: column.name,
                value: column.value,
            })
        } else {
            None
        }
    }
}

impl<'a> Iterable for WideColumns<'a> {
    type Item<'me> = WideColumn<'me>
    where
        Self: 'me;

    type Iter<'me> = WideColumnsIter<'me>
    where
        Self: 'me;

    fn iter(&self) -> Self::Iter<'_> {
        WideColumnsIter {
            columns: self,
            current_idx: 0,
        }
    }
}

impl Drop for WideColumns<'_> {
    fn drop(&mut self) {
        unsafe { ffi::rocksdb_widecolumns_destroy(self.inner.cast_mut()) }
    }
}

impl Drop for PinnableWideColumns<'_> {
    fn drop(&mut self) {
        unsafe { ffi::rocksdb_pinnablewidecolumns_destroy(self.inner.cast_mut()) }
    }
}
