# Sirius blockchain

## Building the Docker release image 

```bash
cd cpp-xpx-chain
docker build -t proximax-sirius-blockchain -f ./scripts/catapult-server-docker/DockerfileDebian .
```

## Run container
```bash
docker run \
    -p 7902:7902 \
    -p 7900:7900 \
    -p 7901:7901 \
    -v /path-to/catapult-config:/catapultconfig \
    -v /path-to/seed-data:/data:rw \
    proximax-sirius-blockchain
```

# Sirius blockchain tools
```bash
cd cpp-xpx-chain
docker build -t proximax-sirius-blockchain-tools -f ./scripts/catapult-server-docker/DockerfileTools .
```
