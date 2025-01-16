# Sirius blockchain

## Building the Docker debug image

```bash
cd cpp-xpx-chain
docker  docker build -t catapult-remote-dbg:v1.0.3-bookworm -f ./scripts/catapult-server-docker/remote-debug/DockerfileDebian-linux-remote-debug .
```

## Connect to remote machine via SSH
```bash
ssh -L 1234:172.22.0.5:1234 -L 2222:172.22.0.5:2222 -i ~/keys ubuntu@109.205.181.31
```

## Run container
### Copy docker compose file to remote machine
```bash
docker compose up
```

## Connect to container
```bash
docker exec -it container_id bash
```

## Run catapult server manually
```bash
./_build/bin/sirius.bc /chainconfig
```

## Find process id
```bash
ps aux | grep sirius.bc
```

## To attach to precess via process id
```bash
gdbserver :1234 --attach <PID>
```

## Clion settings
1. Open Debug -> Edit configurations
2. Add New Configuration -> Remote Debug
3. Debugger -> BundledGDB
4. target remote -> container ip with port(example: 172.21.0.2:1234)
5. Path mapping -> Remote: "/catapult" Local: "cpp-xpx-chain" (root folder of the repository)
6. Save and run debug