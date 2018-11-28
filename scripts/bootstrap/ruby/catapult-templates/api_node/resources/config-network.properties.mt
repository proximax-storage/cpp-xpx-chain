[network]

identifier = {{network_identifier}}
publicKey = {{network_public_key}}
generationHash = {{network_generation_hash}}

[chain]

blockGenerationTargetTime = 15s
blockTimeSmoothingFactor = 3000

# maxTransactionLifetime / blockGenerationTargetTime
importanceGrouping = 5760
maxRollbackBlocks = 360
maxDifficultyBlocks = 3

maxTransactionLifetime = 24h
maxBlockFutureTime = 10s

totalChainBalance = 8'999'999'998'000'000
minHarvesterBalance = 1'000'000'000'000

blockPruneInterval = 360
maxTransactionsPerBlock = 200'000

[plugin:catapult.plugins.aggregate]

maxTransactionsPerAggregate = 1'000
maxCosignaturesPerAggregate = 15

# multisig plugin is expected to do more advanced cosignature checks
enableStrictCosignatureCheck = false
enableBondedAggregateSupport = true

[plugin:catapult.plugins.multisig]

maxMultisigDepth = 3
maxCosignersPerAccount = 10
maxCosignedAccountsPerAccount = 5

[plugin:catapult.plugins.namespace]

maxNameSize = 64

# *approximate* days based on blockGenerationTargetTime
namespaceGracePeriodDuration = 0d
reservedRootNamespaceNames = xem, nem, user, account, org, com, biz, net, edu, mil, gov, info, prx, xpx

namespaceRentalFeeSinkPublicKey = 3E82E1C1E4A75ADAA3CBA8C101C3CD31D9817A2EB966EB3B511FB2ED45B8E262
rootNamespaceRentalFeePerBlock = 100'000
childNamespaceRentalFee = 10'000'000

maxChildNamespaces = 500
maxMosaicsPerAccount = 10'000

isMosaicLevyUpdateAllowed = true
maxMosaicDivisibility = 6
maxMosaicDivisibleUnits = 9'000'000'000'000'000

mosaicRentalFeeSinkPublicKey = 53E140B5947F104CABC2D6FE8BAEDBC30EF9A0609C717D9613DE593EC2A266D3
mosaicRentalFee = 50'000'000

[plugin:catapult.plugins.transfer]

maxMessageSize = 1024

[plugin:catapult.plugins.lock]

lockedFundsPerAggregate = 10'000'000
maxHashLockDuration = 2d
maxSecretLockDuration = 30d
minProofSize = 10
maxProofSize = 1000
