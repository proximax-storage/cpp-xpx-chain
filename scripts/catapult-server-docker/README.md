## Building the Docker release image 

```bash
cd proximax-catapult-server
docker build -t proximax-catapult-server -f ./scripts/catapult-server-docker/Dockerfile .
```

## Run container
```bash
docker run \
    -p 7902:7902 \
    -p 7900:7900 \
    -p 7901:7901 \
    -v /path-to/catapult-config:/catapultconfig \
    -v /path-to/seed-data:/data:rw \
    proximax-catapult-server 
```
