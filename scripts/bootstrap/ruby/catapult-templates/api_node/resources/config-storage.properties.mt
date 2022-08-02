[replicator]

host = {{replicator_host}}
port = {{replicator_port}}
transactionTimeout = 1h
storageDirectory = data/api-node-{{node_index}}/data/drives
sandboxDirectory = data/api-node-{{node_index}}/data/drive-sandboxes
useTcpSocket = true
useRpcReplicator = true
rpcHost = 127.0.0.1
rpcPort = 7905