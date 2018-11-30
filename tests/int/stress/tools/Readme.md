You can build patriciaTreeGenerator by next commands:
```
mkdir temp
cd temp
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-pthread" ..
make publish
make tests.catapult.int.stress.patriciaTreeGenerator -j 4
```

```
./bin/tests.catapult.int.stress.patriciaTreeGenerator --help
Patricia Tree Generator Tool options:
  -h [ --help ]     print help message
  -l [ --loggingConfigurationPath ] arg
                    the path to the logging configuration file
  -i [ --input ] arg (=../tests/int/stress/resources/1.patricia-tree-account.dat)
                    path to input file
  -o [ --output ] arg (=../tests/int/stress/resources/accounts.json)
                    path to output file
  -m [ --mode ] arg (=JsonToBinary)
                    mode { JsonToBinary, BinaryToJson }
```

PatriciaTreeGenerator allow you to generate data for BasePatriciaTreeIntegrityTests.cpp.
It can work in two modes:
 * JsonToBinary, - You need use it, if you want translate json of account to binary format for test.
```
./bin/tests.catapult.int.stress.patriciaTreeGenerator -m jsontobinary -o ../tests/int/stress/resources/3.patricia-tree-account.dat -i ../tests/int/stress/resources/3.accounts.json
```
* BinaryToJson - You need use it, if you want translate binary format of test to json with accounts.
```
./bin/tests.catapult.int.stress.patriciaTreeGenerator -m binarytojson -i ../tests/int/stress/resources/3.patricia-tree-account.dat -o ../tests/int/stress/resources/3.accounts.json
```

