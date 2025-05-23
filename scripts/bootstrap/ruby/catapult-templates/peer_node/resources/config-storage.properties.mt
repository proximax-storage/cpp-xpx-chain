[replicator]

key = {{bootkey}}
host = {{replicator_host}}
port = {{replicator_port}}
transactionTimeout = 1h
storageDirectory = data/peer-node-{{node_index}}/data/drives
sandboxDirectory = data/peer-node-{{node_index}}/data/drive-sandboxes
useTcpSocket = true
useRpcReplicator = true
rpcHost = 127.0.0.1
rpcPort = {{replicator_rpc_port}}
rpcHandleLostConnection = false
rpcDbgChildCrash = true