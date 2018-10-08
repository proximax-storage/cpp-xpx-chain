## Building the Docker release image 

```bash
cd proximax-catapult-server
docker build -t proximax-catapult-tools -f ./scripts/catapult-tools-docker/Dockerfile .
```

## catapult.tools.nemgen
```bash
# Generate raw address in /tmp/raw-addresses.txt
docker run \
    -v /path-to/block-properties-file:/nemesis \
    proximax-catapult-tools \
    /bin/bash -c "/catapult/bin/catapult.tools.nemgen --nemesisProperties /nemesis/block-properties-file.properties"
```

## catapult.tools.address
```bash
# Generate raw address in /tmp/raw-addresses.txt
docker run \
    -v /tmp:/tmp \
    proximax-catapult-tools \
    /bin/bash -c "/catapult/bin/catapult.tools.address --generate=50 -n mijin-test > /tmp/raw-addresses.txt"
```

## catapult.tools.health
```bash
docker run \
    -v /path-to/catapult-config:/catapultconfig \
    proximax-catapult-tools \
    /bin/bash -c "/catapult/bin/catapult.tools.health /catapultconfig"
```

## catapult.tools.network
```bash
# Generate raw address in /tmp/raw-addresses.txt
docker run \
    -v /tmp:/tmp \
    proximax-catapult-tools \
    /bin/bash -c "/catapult/bin/catapult.tools.network /catapultconfig"
```

