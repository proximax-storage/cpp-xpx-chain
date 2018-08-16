Catapult is x64 only, there are no 32-bit builds.

Dependencies:

 * python 3
 * rocksdb: 5.12.4
 * cmake: 3.11.1
 * boost: 1.65.1
 * mongoc 1.4.2
 * mongo-cxx 3.0.2
 * gtest: 1.8.0
 * libzmq: 4.2.3
 * cppzmq: 4.2.3
 * g++-7(Optional, either you can have compilation errors on different platforms)

Variables required to build catapult:

 * `PYTHON_EXECUTABLE`
 * `BOOST_ROOT`
 * `GTEST_ROOT`
 * `LIBBSONCXX_DIR`
 * `LIBMONGOCXX_DIR`
 * `ZeroMQ_DIR`
 * `cppzmq_DIR`
 * win: `RocksDB_DIR`, \*nix: `ROCKSDB_ROOT_DIR`

Once you have all variables set up correctly, build becomes trivial.

Prepare build directory:
```
mkdir _build
cd _build
```

Generate makefiles and build:
```
cmake -DCMAKE_BUILD_TYPE=RelWithDebugInfo ..
make publish
make
```

Or use generator of your choice
```
cmake -DCMAKE_BUILD_TYPE=RelWithDebugInfo -G Ninja ..
ninja publish
ninja
```

VS:
```
cmake -DCMAKE_BUILD_TYPE=RelWithDebugInfo -G "Visual Studio 14 2015 Win64" ..
# Open up catapult_server.sln
```

To create a release docker images you need to install docker first:
```
sudo apt-get install docker-compose -y
```
  Then you need to build an image:
  To build a tools image:
  ```
  cd proximax-catapult-server
  sudo ./scripts/ToolsRealeaseDocker/buildTools.sh
  ```
  To build a catapult.server image:
  ```
  cd proximax-catapult-server
  sudo ./scripts/Catapult.serverRealeaseDocker/buildCatapultServer.sh
  ```
