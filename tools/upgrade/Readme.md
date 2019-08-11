A tool to generate and send blockchain upgrade transaction and blockchain config update transaction.
```
./catapult.tools.upgrade --help

Blockchain Upgrade Tool options:
  -h [ --help ]     print help message
  -l [ --loggingConfigurationPath ] arg (=../resources/config-logging.properties)
                    the path to the logging configuration file
  -s [ --signer-key ] arg
                    nemesis private key required to sign transaction (no default)
  -g [ --generation-hash ] arg
                    nemesis generation hash required to sign transaction (no default)
  -n [ --network ] arg (=mijin-test)
                    network identifier {mijin, mijin-test, private, private-test, public, public-test} (default: mijin-test)
  -b [ --bc-upgrade ]
                    flag to generate blockchain upgrade transaction
  -u [ --bc-upgrade-apply-height-delta ] arg (=360)
                    number of blocks since current chain height to apply blockchain upgrade (default: 360)
  -v [ --bc-version ] arg (=0.0.0.0)
                    new blockchain version (default: 0.0.0.0)
  -c [ --config-update ]
                    flag to generate blockchain config update transaction
  -r [ --resources ] arg (=../resources)
                    path to the resources directory (default: ../resources)
  -d [ --config-apply-height-delta ] arg (=360)
                    number of blocks since current chain height to apply config update (default: 360)
  -h [ --host ] arg (=127.0.0.1)
                    host to connect to (default: 127.0.0.1)
  -p [ --port ] arg (=3000)
                    host port (default: 3000)
  -t [ --host-type ] arg (=rest)
                    type of host {api, rest} (default: rest)
  -a [ --api-key ] arg
                    public key of api node to connect to (default: <empty>)
  -k [ --rest-key ] arg
```

Examples:
 * API
```
./catapult.tools.upgrade --bc-upgrade --bc-upgrade-apply-height-delta 500 --bc-version 1.0.0.0 --config-update --config-apply-height-delta 500 --resources ../../resources --host-type api --host 127.0.0.1 --port 7900 --signer-key 4C469F9A75478661DB2765FFCC3D3AFEA4065A337133C86DFB1A6D9F6EBC8AA7 --generation-hash D13D068AEE2394F5BA26DD41F9A348B9EB053931685BDEFD7F54A5FE315DDF7A --api-key 793717CF811BB89D315D72471457524C5BC020ED8DEEB4CF478563C586C55B20 --rest-key F55EDFAE222AB0375536583C11C538636452E1ECEAE62640780A045AEB0ABE0A
```
* REST
```
./catapult.tools.upgrade --bc-upgrade --bc-upgrade-apply-height-delta 500 --bc-version 1.0.0.0 --config-update --config-apply-height-delta 500 --resources ../../resources --host-type rest --host 127.0.0.1 --port 3000 --signer-key 4C469F9A75478661DB2765FFCC3D3AFEA4065A337133C86DFB1A6D9F6EBC8AA7 --generation-hash D13D068AEE2394F5BA26DD41F9A348B9EB053931685BDEFD7F54A5FE315DDF7A
```

Tips:
* The resources directory should contain blockchain configuration files *config-network.properties* and *supported-entities.json*.
