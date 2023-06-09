# B+Tree Library "Beet"

Beet is a B+Tree implementation that can be used to build
disk-backed key-value storage structures like large hash maps or indices for databases.
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
ordered and allows for quick searches and range scans on data level.
The feature can be used, for example, to implement secondary indices
for databases where keys store data file offsets where
the data corresponding to the key can be found.

## API

### Index Creation

Indices are created by means of the create service:

```C
    beet_err_t beet_index_create(const char *base,     // the base path to the directory where the index is created
                                 const char *path,     // the path to the index relative to the base path
                                 char standalone,      // embedded are all index types excpet embedded indices
                                 beet_config_t *cfg);  // pointer to the config (see below)
```

The function receives a base path and a path relative to the base.
The intention is to ease the management of indices that belong together, e.g. a host and an embedded index.
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
beet_err_t beet_index_drop(const char *base, const char *path);
```

Note that the directories in the `path` parameter are removed.

### Opening and Closing an Index

An index, once created, can be opened by means of the open service:

```C
beet_err_t beet_index_open(const char *base,
                           const char *path,   // base and path components
                           void       *handle, // comparison library handle
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
and pass the handle to the function. Note that, if you open a host index
that contains an embedded index, the comparison functions for the data types
in the host index and in the embedded index must be found in that library
(or in the code of your main program). The names of the functions are
provided in the `create` configuration (see [config](#Config)).

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
    fprintf(stderr, "cannot open index: %s\n", beet_errdesc(err));
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

#### The Create Config

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
and for internal nodes (which contain pointers to other nodes).

The attributes `leafNodeSize` and `intNodeSize` indicate
the number of key/value pairs stored in one node of the corresponding type.

The attributes `leafPageSize` and `intPageSize` indicate the size
of pages that store leaf nodes and internal nodes respectively in bytes.
These values are computed internally based on node, key and data size.
Application code does not need to bother with these settings.

The attributes `keySize` and `dataSize` indicate the size of one key
and one data record respectively.

The Beet library uses caches to retrieve nodes from disk.
One cache is exclusively used for leaf nodes and one is used only for internal nodes.
The attributes `leafCacheSize` and `intCacheSize` indicate the size of these caches
in terms of the number of nodes to be kept in memory.
There are some special values:

- BEET_CACHE_UNLIMITED: the cache grows without upper bound
- BEET_CACHE_DEFAULT  : an internal default is used
- BEET_CACHE_IGNORE   : has meaning in the context with the `open` config (see below).

The attribute `subPath` contains the path to the embedded index.
If the index is not a host, this attribute must be `NULL`.

The `compare` attribute contains the name of the `compare` function
for the keys of this index. The `compare` function may be defined
anywhere in the code of the calling program or in a special library
which is then passed to the `open` service as `handle`.
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
This object is passed in with the `open` config and stored internally.
It can be retrieved explicitly by means of the `getResource` service:

```C
void *beet_index_getResource(beet_index_t idx);
```

The `compare` function can also be retrieved explicitly using the `getCompare` service:

```C
beet_compare_t beet_index_getCompare(beet_index_t idx);
```

The last two attributes of the config structure (`rscinit` and `rscdest`) are names of functions
(defined in the code of the calling program or the already mentioned special library)
that initialise this object or destroy it at the end.
If these attributes are `NULL`, no initialisation or destruction is performed.

#### The Open Config

The purpose of the `open` config is to overwrite some of the settings chosen at creation time
for a specific session, be it for debugging or be it for finding optimal settings interactively.
But it also sets one value that is not considered in the `create` config,
namely the user defined resource.
The `open` config has the following format:

```C
typedef struct {
    int32_t  leafCacheSize; // cache size for leaf nodes
    int32_t   intCacheSize; // cache size for internal nodes
    beet_compare_t compare; // pointer to compare function
    beet_rscinit_t rscinit; // pointer to rsc init function
    beet_rscinit_t rscdest; // pointer to rsc destroyer function
    void              *rsc; // passed in to rscinit
} beet_open_config_t;
```

The first two values are the cache size settings we already saw in the `create` config.
If they are set to any value different from `BEET_CACHE_IGNORE`,
the values in the `create` config are overwritten.
This is in particular useful for optimising and fine tuning index performance.

The `compare` attribute is a pointer to a `compare` function.
If the value is different from NULL it is used instead of the symbol stored in the `create` config.
This is useful for debugging.
Note, however, that, when you have a host index and an embedded index with different key types,
this approach won't not work, since we need two different `compare` functions.

The next two attributes, `rcsinit` and `rcsdest`
overwrite the symbols of the same name in the `create` config.

Finally, \*rsc is an arbitrary object that can be passed in to be used with `compare`.

Since we, usually, want to ignore most attributes of the `open` config,
there is a handy function that initialises an `open` config with all values ignored:

```C
void beet_open_config_ignore(beet_open_config_t *cfg);
```

### Inserting

Key/value pairs are inserted with the `insert` service:

```C
beet_err_t beet_index_insert(beet_index_t idx, const void *key, const void *data);
```

The function receives the index into which to insert the pair, a pointer to the key
and a pointer to the data.
If the key does not exist, it is inserted.
If the key already exists, the function returns the error `BEET_ERR_DBLKEY`.
An alternative to `insert` is the `upsert` service:

```C
beet_err_t beet_index_upsert(beet_index_t idx, const void *key, const void *data);
```

The `upsert` service works exactly like `insert`, with the exception that it does not return an error
if the key exists. Instead it silently overwrites the data.

The following code snippet would insert data into an index with keys of type `uint64_t` and data `double`:

```C
uint64_t k = 100;
double   d = 3.14159;

err = beet_index_insert(idx, &k, &d);
if (err != BEET_OK) {
    fprintf(err, "cannot insert into index: %s\n", beet_errdesc(err));
    return EXIT_FAILURE;
}
```

When inserting into an index with an embedded tree, the data is a key/value pair itself
and must be of type `beet_pair_t`:

```C
typedef struct {
    void *key;
    void *data;
} beet_pair_t;

```

Here is an example:

```C
uint64_t k = 12;

for (uint64_t z=1; z<k; z++) {
    uint64_t r = k%z;
    beet_pair_t p;
    p.key  = &z;
    p.data = &r;
    beet_err_t err = beet_index_insert(idx, &k, &p);
    if (err != BEET_OK) {
        fprintf(stderr, "cannot insert into index: %s\n", beet_errdesc(err));
        return EXIT_FAILURE;
    }
}
```

We can check that a certain key was inserted into the tree by means of the `exist` service:

```C
beet_err_t beet_index_doesExist(beet_index_t idx, const void *key);
```

If the function returns `BEET_OK`, the key exists.
If it does not exist, the function returns `BEET_ERR_KEYNOF`.

To verify that a key exists in an embedded index we can use:

```C
beet_err_t beet_index_doesExist2(beet_index_t idx, const void *key1, const void *key2);
```

### Searching

The simplest way to retrieve data from an index is the `copy` service:

```C
beet_err_t beet_index_copy(beet_index_t idx, const void *key, void *data);
```

If the key is found, the function copies the corresponding record into the location
referenced by data. If the key is not found, the function returns the error `BEET_ERR_KEYNOF`:

```C
uint64_t k = 100;
double d;

beet_err_t err = beet_index_copy(idx, &k, &d);
if (err == BEET_ERR_KEYNOF) {
    fprintf(stderr, "key %lu not found\n", k);
    return EXIT_SUCCESS;
} else if (err != BEET_OK) {
    fprintf(stderr, "cannot retrieve data for key %lu: %s\n", k, beet_errdesc(err));
    return EXIT_FAILURE;
}
fprintf(stdout, "%lu: %f\n", k, d);
```

To copy data from an embedded index, we can use the following service:

```C
beet_err_t beet_index_copy2(beet_index_t idx, const void *key1, const void *key2, void *data);
```

Copying the data is not always the best solution, for example in cases where
many retrievals are performed and the data is large.
For such cases, the `get` service is available:

```C
beet_err_t beet_index_get(beet_index_t  idx,
                          beet_state_t  state,
                          uint16_t      flags,
                          const void   *key,
                          void        **data);
```

The second parameter `state` keeps track of the internal state, in particular
`get` needs to maintain the node that contains the key and its data in memory
and locks it from write access during this time. Therefore, the state should be released
as soon a possible.

The parameter `flags` should almost always be set to 0.
There are technical use cases where it makes sense to use other flags.
But we don't cover such cases in the scope of this intro.

Finally, the pointer `data` is set to the memory address of the data under the key.
Overwriting the data at this position directly is **dangerous**:
it may lead to race conditions with other readers not to mention bugs
that may corrupt the index.

Retrieving data from an embedded index is done with the `get2` service:

```C
beet_err_t beet_index_get2(beet_index_t  idx,
                           beet_state_t  state,
                           uint16_t      flags,
                           const void   *key1,
                           const void   *key2,
                           void        **data);
```

The second `key` parameter is the key of the embedded index.
Otherwise, the function behaves exactly like `get`.

Here is a usage example:

```C
beet_state_t state;
err = beet_state_alloc(idx, &state); // allocate the state
if (err != BEET_OK) {
    fprintf(stderr, "cannot allocate state: %s\n", beet_errdesc(err));
    return EXIT_FAILURE;
}
uint64_t k = 12;

for(uint64_t z=1; z <= k; z++) {
    uint64_t *r;
    err = beet_index_get2(idx, state, 0, &k, &z, &r);
    if (err != BEET_OK) {
       fprintf(stderr, "cannot retrieve data of %lu.%lu: %s\n", k, z, beet_errdesc(err));
       err = beet_state_release(state); // release all locks
       if (err != BEET_OK) {
           fprintf(stderr, "PANIC: cannot release state: %s\n", beet_errdesc(err));
           return EXIT_FAILURE;
       }
       beet_state_destroy(state); // free memory
       return EXIT_FAILURE;
    }
    fprintf(stdout, "The remainder of %lu and %lu is %lu\n", k, z, *r);
    err = beet_state_release(state); // release all locks
    if (err != BEET_OK) {
        fprintf(stderr, "PANIC: cannot release state: %s\n", beet_errdesc(err));
        return EXIT_FAILURE;
    }
    beet_state_reinit(state);  // clean state
}
beet_state_destroy(state); // free memory
```

### Iterators

An additional way to retrieve data is provided by iterators.
Iterators are especially useful with range scans
where we iterate over a sequence of keys.
We start by allocating an iterator:

```C
beet_err_t beet_iter_alloc(beet_index_t idx,
                           beet_iter_t *iter);
```

We then perform a range scan:

```C
beet_err_t beet_index_range(beet_index_t        idx,
                            const beet_range_t *range,
                            beet_dir_t          dir,
                            beet_iter_t         iter);
```

The second parameter, `range`, defines the key range in which we search.
The type is:

```C
typedef struct {
    void *fromkey;
    void *tokey;
} beet_range_t;
```

The third parameter indicates the direction of the search, which is either

- `BEET_DIR_ASC` (for ascending) or
- `BEET_DIR_DESC` (for descending).

The last parameter is the already allocated iterator.

When the function terminated successfully, we can move the iterator
through the key range:

```C
beet_err_t beet_iter_move(beet_iter_t iter, void **key, void **data);
```

The function sets the iterator to the next key
(which is the first key in the range on first call)
and sets the pointers key and data to the data in the node.
As with the `get` services, the node remains in memory and is locked
from concurrent write access until we move to the next node in the range.
Range scan operations shall therefore be fast.

If we have already reached the last node in the range,
the function returns `BEET_ERR_EOF`.

When iterating over a key range of a host index,
we can enter the embedded index by means of the `enter` service:

```C
beet_err_t beet_iter_enter(beet_iter_t iter);
```

When we now `move` we pass to the next key/value pair in the embedded index.

The `leave` service switches back to the range of the host index:

```C
beet_err_t beet_iter_leave(beet_iter_t iter);
```

Here is a complete usage example:

```C
beet_err_t err;
uint64_t *k, *z, *r, *d;
uint64_t from, to;
beet_range_t range;
beet_iter_t iter;

from = 1;
to = 12;
range.fromkey = &from;
range.tokey = &to;

err = beet_iter_alloc(idx, &iter);
if (err != BEET_OK) {
    fprintf(stderr, "cannot create iter: %s\n", beet_errdesc(err));
    return EXIT_FAILURE;
}
err = beet_index_range(idx, &range, BEET_DIR_ASC, iter);
if (err != BEET_OK) {
    fprintf(stderr, "cannot initiate iter: %s\n", beet_errdesc(err));
    beet_iter_destroy(iter);
    return EXIT_FAILURE;
}
while((err = beet_iter_move(iter, (void**)&k,
                                  (void**)&d)) == BEET_OK)
{
    err = beet_iter_enter(iter);
    if (err != BEET_OK) {
        fprintf(stderr, "cannot enter key %lu: %s\n", k, beet_errdesc(err));
        beet_iter_destroy(iter);
        return EXIT_FAILURE;
    }

    while((err = beet_iter_move(iter, (void**)&z,
                                      (void**)&r)) == BEET_OK)
    {
        fprintf(stdout, "The remainder of %lu and %lu is %lu\n", *k, *z, *r);
    }
    if (err != BEET_ERR_EOF) {
        fprintf(stderr, "cannot move through key %lu: %s\n", k, beet_errdesc(err));
        beet_iter_destroy(iter);
        return EXIT_FAILURE;
    }

    err = beet_iter_leave(iter);
    if (err != BEET_OK) {
        fprintf(stderr, "cannot leave key %lu: %s\n", k, beet_errdesc(err));
        beet_iter_destroy(iter);
        return EXIT_FAILURE;
    }
}
beet_iter_destroy(iter); // free memory
if (err != BEET_ERR_EOF) {
    fprintf(stderr, "cannot move through range: %s\n", beet_errdesc(err));
    return EXIT_FAILURE;
}
```

It is also possible to range through the keys of a subindex.
For this we first need to obtain the subindex from a key in the host index.
This is done by means of the `getIter` service:

```C
beet_err_t beet_index_getIter(beet_index_t        idx,
                              const beet_range_t *range,
                              beet_state_t        state,
                              const void         *key,
                              beet_iter_t         iter);
```

The first parameter is the main index.
The second parameter is a range. If the range is `NULL`,
we will iterate through all keys in the subindex.
The third parameter is a state just as in the `get` services.
The `key` parameter refers to the key in the host index and
`iter` is an already allocated iterator.
Here is an example:

```C
beet_state_t state;
beet_iter_t  iter;
beet_range_t range;
uint64_t k, f, t;
uint64_t *z, *r;

err = beet_state_alloc(idx, &state);
if (err != BEET_OK) {
    fprintf(stderr, "could not allocate state: %s\n", beet_err_desc(err));
    return EXIT_FAILURE;
}
err = beet_iter_alloc(idx, &iter);
if (err != BEET_OK) {
    fprintf(stderr, "could not allocate iter: %s\n", beet_err_desc(err));
    beet_state_destroy(state);
    return EXIT_FAILURE;
}

k=12; f=6; t=9;
range.fromkey = &f;
range.tokey = &t;
err = beet_index_getIter(idx, &range, state, &k, iter);
if (err != BEET_OK) {
    fprintf(stderr, "could not obtain iter: %s\n", beet_err_desc(err));
    beet_iter_destroy(iter);
    beet_state_destroy(state);
    return EXIT_FAILURE;
}

fprintf(stdout, "The remainders of %ul, > %ul, < %ul are:\n", k, f-1, t+1);
while((err = beet_iter_move(iter, (void**)&z, (void**)&r)) == BEET_OK) {
    fprintf(stdout, "%ul -> %ul\n", *z, *r);
}
if (err != BEET_ERR_EOF) {
    fprintf(stderr, "cannot move through key %lu: %s\n", k, beet_errdesc(err));
    beet_iter_destroy(iter);
    beet_state_destroy(state);
    return EXIT_FAILURE;
}
beet_iter_destroy(iter); // free memory
beet_state_destroy(state); // free memory
```

### Deleting and Hiding

The `delete` service deletes a key and its data from the index:

```C
beet_err_t beet_index_delete(beet_index_t idx, const void *key);
```

To delete a key and its data from an embedded index the following service can be used:

```C
beet_err_t beet_index_delete2(beet_index_t idx, const void *key1, const void *key2);
```

Since deleting is a costly operation, there is an alternative approach.
Keys can be hidden, so that retrieval operations won't find them.
To hide a key in the index the `hide` service is used:

```C
beet_err_t beet_index_hide(beet_index_t idx, const void *key);
```

To hide a key in an embedded index, `hide2` is used:

```C
beet_err_t beet_index_hide2(beet_index_t idx, const void *key1, const void *key2);
```

When a hidden key is later inserted again,
it is simply uncovered, so that it is visible again.

To avoid that the tree continues growing,
hidden keys can be removed by an incremental background job
that would call delete on every hidden key:

```C
beet_err_t beet_index_purge(beet_index_t idx, int runtime);
```

The second parameter of `purge` is the maximal running time
of the service in seconds. If the function has run for more
than the specified number of seconds, it will terminate.

## Testing

The library is tested on Linux and should work
on other POSIX systems as well. The tests use features
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

Beet depends complies to C99
and depends on the [tsalgo](https://github.com/toschoo/tsalgo) library.

## TODOs and Bugs

- delete and purge are not yet implemented
