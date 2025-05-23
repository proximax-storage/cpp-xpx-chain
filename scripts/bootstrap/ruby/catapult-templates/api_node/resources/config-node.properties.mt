[node]

port = {{port}}
apiPort = {{api_port}}
dbrbPort = {{dbrb_port}}
shouldAllowAddressReuse = false
shouldUseSingleThreadPool = false
shouldUseCacheDatabaseStorage = true
shouldEnableAutoSyncCleanup = true

shouldEnableTransactionSpamThrottling = true
transactionSpamThrottlingMaxBoostFee = 10'000'000

maxBlocksPerSyncAttempt = 400
maxChainBytesPerSyncAttempt = 100MB

shortLivedCacheTransactionDuration = 10m
shortLivedCacheBlockDuration = 100m
shortLivedCachePruneInterval = 90s
shortLivedCacheMaxSize = 10'000'000

minFeeMultiplier = 0
feeInterest = 1
feeInterestDenominator = 1
rejectEmptyBlocks = false

transactionSelectionStrategy = oldest
unconfirmedTransactionsCacheMaxResponseSize = 20MB
unconfirmedTransactionsCacheMaxSize = 1'000'000

connectTimeout = 10s
syncTimeout = 60s

socketWorkingBufferSize = 512KB
socketWorkingBufferSensitivity = 100
maxPacketDataSize = 150MB

blockDisruptorSize = 16384
blockElementTraceInterval = 1
transactionDisruptorSize = 65536
transactionElementTraceInterval = 10

shouldAbortWhenDispatcherIsFull = true
shouldAuditDispatcherInputs = true

outgoingSecurityMode = None
incomingSecurityModes = None

maxCacheDatabaseWriteBatchSize = 5MB
maxTrackedNodes = 5'000

transactionBatchSize = 50

[localnode]

host = {{host}}
friendlyName = {{friendly_name}}
version = 0
roles = Api

[outgoing_connections]

maxConnections = 10
maxConnectionAge = 5
maxConnectionBanAge = 20
numConsecutiveFailuresBeforeBanning = 3

[incoming_connections]

maxConnections = 512
maxConnectionAge = 10
backlogSize = 512
maxConnectionBanAge = 20
numConsecutiveFailuresBeforeBanning = 3
