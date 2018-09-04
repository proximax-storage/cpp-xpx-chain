You can build spammer by next commands:
```
mkdir temp
cd temp
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-pthread" ..
make publish
make spammer -j4
```

To run spammer:
```
./bin/spammer --help
Allowed options:
  --help                  produce help message
  --host arg (=127.0.0.1) Ip to connect
  --port arg (=3000)      Port to connect
  --total arg (=10)       Total count of transactions
  --rate arg (=1)         Rate transactions per second
  --mode arg (=rest)      Mode of connection: {rest, node}
  --apiNodePublicKey arg  Public key of api node
  --restPrivateKey arg    Private key verify connection to api node
  --privateKeys arg       Private keys of accounts with tokens
  --token arg (=prx:xpx)  Tokens that you want to transfer
  --value arg (=1)        Amount of tokens that you want to transfer
```

Spammer generates 3 500 - 4 500 transactions per second.
It can work in two modes:
 * node - You need use it, if you want connect to Api Node. But, you need to pass apiNodePublicKey and restPrivateKey(it is need to pass validation challenge).
```
./bin/spammer --privateKeys F9051FFB354BBF7E4D0F3650E3A26EF697FDB96541F61A9D4EC0F679F46EA24A --total 10000 --rate 1000 --host 127.0.0.1 --port 7900 --apiNodePublicKey C946570D2A4842D80140C744F7C9286B39FAEB7B2B94D811963D34F38832B886 --restPrivateKey 91CD243FC24FC000DC52127CBA3AD9372DDBE478BD69F2C53730E1D29A320EDA --mode node  --token nem:xem --value 1000
```
* rest - You need use it, if you want connect to Rest server.
```
./bin/spammer --privateKeys F9051FFB354BBF7E4D0F3650E3A26EF697FDB96541F61A9D4EC0F679F46EA24A --total 10000 --rate 1000 --host 127.0.0.1 --port 3000 --mode rest
```

Tips:
* If you spam to rest server and rate it to high, you can get socket error. Max rate for rest server is 800 tps.
* Max tps for api node is ~3 000.
