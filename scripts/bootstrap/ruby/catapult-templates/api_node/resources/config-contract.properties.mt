[executor]

key = {{bootkey}}
transactionTimeout = 1h
storageRPCHost = 127.0.0.1
storageRPCPort = {{replicator_services_port}}
messengerRPCHost = 127.0.0.1
messengerRPCPort = {{replicator_services_port}}
virtualMachineRPCHost = 127.0.0.1
virtualMachineRPCPort = 50051
useRPCExecutor = false
executorRPCHost = 127.0.0.1
executorRPCPort = {{executor_rpc_port}}
executorLogPath = /tmp/executor/executor.log