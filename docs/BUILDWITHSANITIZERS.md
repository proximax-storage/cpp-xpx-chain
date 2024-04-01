# Sanitizers

**NOTE**: Only one of them can be enabled simultaneously

## Address 

```sh
mkdir _build && cd _build
cmake -DCMAKE_BUILD_TYPE=Debug -DDO_NOT_SKIP_BUILD_TESTS=TRUE -DSANITIZE_ADDRESS=ON ..
make -j 5
```

## Thread 

```sh
mkdir _build && cd _build
cmake -DCMAKE_BUILD_TYPE=Debug -DDO_NOT_SKIP_BUILD_TESTS=TRUE -DSANITIZE_THREAD=ON ..
make -j 5
```

## Undefined

```sh
mkdir _build && cd _build
cmake -DCMAKE_BUILD_TYPE=Debug -DDO_NOT_SKIP_BUILD_TESTS=TRUE -DSANITIZE_UNDEFINED=ON ..
make -j 5
```

If thre are issues with rocksDb try to re-build rocksDb lib with RTTI:
```
make EXTRA_CXXFLAGS=-fPIC EXTRA_CFLAGS=-fPIC USE_RTTI=1 DEBUG_LEVEL=0
```

## Memory 

Before the build some c++ std libs and google tests should be rebuild with MemorySanitizer too. See https://github.com/google/sanitizers/wiki/MemorySanitizerLibcxxHowTo

```sh
mkdir _build && cd _build
cmake -DCMAKE_BUILD_TYPE=Debug -DDO_NOT_SKIP_BUILD_TESTS=TRUE -DSANITIZE_MEMORY=ON ..
make -j 5
```