# B+Tree Library "Beet"

Beet is a B+Tree implementation that can be used to build
disk-backed key-data storage structures like large hash maps or indices for databases.
It provides an API with services to

- create, open, close and destroy indices
- insert keys and data into an existing index
- lookup keys
- iterate over key ranges
- delete single data records and keys
- define key comparison methods.

Any data type with an upper-bound size can be used as keys and data.
One key may store one data record or a list of records. For the second case,
the B+Tree data structure is repeated on data level, i.e. the data
is the root into another B+Tree. This ensures that the list is kept
ordered and allows for quick searches and range iterations on data level.
The feature can be used, for example, to implement secondary indices
for databases where keys store references to data file offsets where
the data corresponding to the key can be found.

## API

### Index Creation

Indices are created by means of the create service:

```C
    beet_err_t beet_index_create(char *base,           // the base path to the directory where the index is created
                                 char *path,           // the path to the index relative to the base path
                                 char standalone,      // embedded are all index types excpet embedded indices
                                 beet_config_t *cfg);  // pointer to the config (see below)
```

The function receives a base path and a path relative to this path.
The intention is the ease the management of indices that belong together, e.g. a host and an embedded index.
They would share the same base path but use different subdirectories at that location.

The `standalone` flag indicates whether or not the index shall be used as an index embedded into another.
Indices that are not embedded should be created as `standalone`, i.e. with a value different from 0,
while embedded indices are created with 0.

The `cfg` parameter contains creation options related to index type and sizing. For details please refer
to the [config](#Config) section.

Here is an example of how to call the function:

```C
err = beet_index_create("/opt/data/index/myindex",
                        "host", 1, &config);
if (err != BEET_OK) {
    fprintf(stderr, "cannot create index 'myindex': %s\n", beet_errdesc(err));
    return -1;
}
```

This creates an index called `myindex` in the directory `/opt/data/index/myindex`.
If `myindex` does not exist in `/opt/data/index`, it is created.
Within the `myindex` directory, there will be a directory called `host`.

To remove an index, use the function

```C
beet_err_t beet_index_drop(char *base, char *path);
```

Note that the directories in the `path` parameter are removed.

### Open and Closing an Index

An index, once created, can be opened by means of the open service:

```C
beet_err_t beet_index_open(char *base, char   *path, // base and path components
                           void             *handle, // comparison library handle
                           beet_open_config_t  *cfg, // opening config
                           beet_index_t       *idx); // pointer to the index object
```

As the `create` and `drop` services `open` identifies the index by means of two path components,
`base` and `path`.

The `handle` parameter is treated as a pointer to the comparison library,
a shared library that contains the code for the comparison functions
you want to use with your data types. If the comparison functions
are implemented in your main program or in a library that is loaded anyway,
the handle may be `NULL`. Otherwise, you need to open that library first
and pass the handle to the function.

The `cfg` parameter is a pointer to the `open` configuration which is used
to provide additional information not stored in the `create` configuration
and to overwrite some of the values.
For details, please refer to the [config](#Config) section.

Here is an example of how to open an existing index:

```C
handle = beet_lib_init("libcmp.so");
if (handle == NULL) {
    fprintf(stderr, "cannot load library\n");
    return EXIT_FAILURE;
}

err = beet_index_open(base, path, handle, &cfg, &idx);
if (err != BEET_OK) {
    errmsg(err, "cannot open index: %s\n", beet_err_desc(err));
    return EXIT_FAILURE;
}
```

The first call opens the library (by means of `dlopen`).
The second call opens the index providing the two path components,
the handle, an `open` config and a pointer where the index object
is stored.

An index is closed by means of the `close` service:

```C
void beet_index_close(beet_index_t idx);
```

### Config

There are two kinds of configurations:

- The `create` config and
- The `open` config.

The first is used with the `create` service when the index is created.
It has the following structure:

```C
typedef struct {
    uint32_t indexType;     // index type
    uint32_t leafPageSize;  // page size of leaf nodes
    uint32_t intPageSize;   // page size of internal nodes
    uint32_t leafNodeSize;  // number of keys in leaf nodes
    uint32_t intNodeSize;   // number of keys in internal nodes
    uint32_t keySize;       // size of one key
    uint32_t dataSize;      // data size
    int32_t  leafCacheSize; // cache size for leaf nodes
    int32_t  intCacheSize;  // cache size for internal nodes
    char    *subPath;       // path to the embedded index
    char    *compare;       // name of compare function
    char    *rscinit;       // name of rsc init function
    char    *rscdest;       // name of rsc destroyer function
} beet_config_t;
```

Index types are:

- BEET_INDEX_NULL : an index without data (key-only)
- BEET_INDEX_PLAIN: a normal index with key and data
- BEET_INDEX_HOST : an index with an embedded index.

Note that an embedded index can only be of type `NULL` or `PLAIN`.
In other words, the embedding is limited to two levels. One cannot
embed an index into an embedded index.

The next six attributes describe the sizing of nodes and pages.
Page is the storage unit of node. For each sizing level
(page and node) there are two values: for leaf nodes (which contain the data)
and internal nodes (which contain pointers to other nodes as data).

The attributes `leafNodeSize` and `intNodeSize` indicate
the number of key/data pairs stored in one node of the corresponding type.

The attributes `leafPageSize` and `intPageSize` indicate the size
of pages that store leaf nodes and internal node respectively in bytes.

The size of a leaf page must be at least:

`keySize * leafNodeSize + dataSize * leafNodeSize + 12 + leafNodeSize/8 + 1`.

The size of an internal page must be at least:

`keySize * intNodeSize + 4 * intNodeSize + 4 + 4`.

Currently these sizes are not computed internally, but must be given
in the configuration and *must* be correct. In a future version,
the page size settings will be ignored and calculated internally.

The attributes `keySize` and `dataSize` indicate the size of one key
and one data record respectively.

The Beet library uses caches to retrieve nodes from disks.
One cache is exclusively used for leaf nodes and one is used only for internal nodes.
The attributes `leafCacheSize` and `intCacheSize` indicate the size of these caches
in terms of numbers of nodes that can be stored.
There are two special values:

- BEET_CACHE_UNLIMITED (0) : the cache grows without upper bound
- BEET_CACHE_DEFAULT   (-1): an internal default is used.

The attribute `subPath` contains the path to the embedded index.
If the index is not a host, this attribute must be `NULL`.

The `compare` attribute contains the name of the compare function
for the keys of this index. The compare function may be defined
anywhere in the code of the calling program or in a special library
which is than passed to the `open` service as `handle`.
The `compare` function must have the following type:

```C
typedef char (*beet_compare_t)(const void*, const void*, void*);
```

The `const void*` parameters are the left and the right value to be compared.
The expected behaviour is:

- If they are equal, the function shall return `BEET_CMP_EQUAL` (0).
- If left < right, the function shall return `BEET_CMP_LESS` (-1).
- If left > right, the function shall return `BEET_CMP_GREATER` (1).

The third parameter of the function is a pointer to an object (`rsc`)
only known to user code which may be used for comparison.
This object is stored internally and passed on the comparison function.
The last two attributes of the config structure (`rscinit` and `rscdest`) are names of functions
(defined in the code of the calling program or the already mentioned special library)
that initialise this object or destroy it at the end.
If these attributes are `NULL`, no initialisation or destruction is performed.

### Inserting and Searching

### Iterators

### Hiding and Deleting

## Testing

The library is tested on Linux and should work
on other systems as well. The tests use features
only available on Linux (e.g. high resolution timers)
and won't work on CygWin (and similar).

## Licence

The code is licenced under the LGPL V3 with the exception
that static linking is explicitly allowed
("static linking exception").

## Building, Installing and Dependencies

The library comes with a GNU Makefile.

- `make`
  builds the library and tests

- `make run`
  runs the tests

- `make all`
   builds the library, tests and tools

- `make debug`
  like `make all` but in debug mode

- `make tools`
   builds the library and tools

- `make lib`
  builds the library only

- `make clean`
  removes all binaries and object files

- `. setenv.sh`
  adds `./lib` to the `LD_LIBRARY_PATH`

Beet depends on C99 and the [tsalgo](https://github.com/toschoo/tsalgo) library.
