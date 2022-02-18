[network]

publicKey = {{network_public_key}}

[chain]

blockGenerationTargetTime = 10s
blockTimeSmoothingFactor = 1000

greedDelta = 0.5
greedExponent = 3.2

importanceGrouping = 5760
maxRollbackBlocks = 360
maxDifficultyBlocks = 3

maxTransactionLifetime = 24h
maxBlockFutureTime = 10s

maxMosaicAtomicUnits = 9'000'000'000'000'000

totalChainImportance = 8'999'999'998'000'000
minHarvesterBalance = 100'000'000'000
harvestBeneficiaryPercentage = 10

blockPruneInterval = 360
maxTransactionsPerBlock = 200'000

enableUnconfirmedTransactionMinFeeValidation = true

enableUndoBlock = true
enableBlockSync = true

enableWeightedVoting = false
committeeSize = 21
committeeApproval = 0.67
committeePhaseTime = 5s
minCommitteePhaseTime = 375ms
maxCommitteePhaseTime = 1m
committeeSilenceInterval = 100ms
committeeRequestInterval = 500ms
committeeChainHeightRequestInterval = 30s
committeeTimeAdjustment = 1.1
committeeEndSyncApproval = 0.45
committeeBaseTotalImportance = 100
committeeNotRunningContribution = 0.5

[plugin:catapult.plugins.accountlink]
dummy = to trigger plugin load

[plugin:catapult.plugins.aggregate]

maxTransactionsPerAggregate = 1'000
maxCosignaturesPerAggregate = 15

# multisig plugin is expected to do more advanced cosignature checks
enableStrictCosignatureCheck = false
enableBondedAggregateSupport = true

maxBondedTransactionLifetime = 48h
strictSigner = true

[plugin:catapult.plugins.committee]

enabled = true

minGreed = 0.1
# log(7/3)
initialActivity = 0.367976785
activityDelta = 0.00001
activityCommitteeCosignedDelta = 0.01
activityCommitteeNotCosignedDelta = 0.02

[plugin:catapult.plugins.config]

maxBlockChainConfigSize = 1MB
maxSupportedEntityVersionsSize = 1MB

[plugin:catapult.plugins.exchange]

enabled = true

maxOfferDuration = 57600
longOfferKey = CFC31B3080B36BC3D59DF4AB936AC72F4DC15CE3C3E1B1EC5EA41415A4C33FEE

[plugin:catapult.plugins.lockhash]

lockedFundsPerAggregate = 10'000'000
maxHashLockDuration = 2d

[plugin:catapult.plugins.locksecret]

maxSecretLockDuration = 30d
minProofSize = 1
maxProofSize = 1000

[plugin:catapult.plugins.metadata]

maxFields = 10
maxFieldKeySize = 128
maxFieldValueSize = 1024

[plugin:catapult.plugins.mosaic]

maxMosaicsPerAccount = 10'000
maxMosaicDuration = 3650d
maxMosaicDivisibility = 6

mosaicRentalFeeSinkPublicKey = 53E140B5947F104CABC2D6FE8BAEDBC30EF9A0609C717D9613DE593EC2A266D3
mosaicRentalFee = 10'000'000'000

[plugin:catapult.plugins.multisig]

maxMultisigDepth = 3
maxCosignersPerAccount = 10
# 2^20
maxCosignedAccountsPerAccount = 1048576

newCosignersMustApprove = true

[plugin:catapult.plugins.namespace]

maxNameSize = 64

# *approximate* days based on blockGenerationTargetTime
maxNamespaceDuration = 365d
namespaceGracePeriodDuration = 0d
reservedRootNamespaceNames = xem, nem, user, account, org, com, biz, net, edu, mil, gov, info, prx, xpx, xarcade, xar, proximax, prc, storage

namespaceRentalFeeSinkPublicKey = 3E82E1C1E4A75ADAA3CBA8C101C3CD31D9817A2EB966EB3B511FB2ED45B8E262
rootNamespaceRentalFeePerBlock = 10'000'000'000
childNamespaceRentalFee = 10'000'000'000

maxChildNamespaces = 500

[plugin:catapult.plugins.operation]

enabled = true

maxOperationDuration = 2d

[plugin:catapult.plugins.property]

maxPropertyValues = 512

[plugin:catapult.plugins.transfer]

maxMessageSize = 1024
maxMosaicsSize = 512

[plugin:catapult.plugins.upgrade]

minUpgradePeriod = 360

[plugin:catapult.plugins.service]

enabled = true

maxFilesOnDrive = 32768
verificationFee = 10
verificationDuration = 240
downloadDuration = 40320

downloadCacheEnabled = true

[plugin:catapult.plugins.supercontract]

enabled = true
maxSuperContractsOnDrive = 10

[plugin:catapult.plugins.storage]

enabled = true
minDriveSize = 1MB
maxDriveSize = 10TB
maxModificationSize = 10TB
minReplicatorCount = 1
maxFreeDownloadSize = 1MB
maxDownloadSize = 10TB
# 4 weeks = 28 days = 672 hours
storageBillingPeriod = 672h
downloadBillingPeriod = 24h
verificationInterval = 4h
shardSize = 20
verificationExpirationCoefficient = 0.06
verificationExpirationConstant = 10

[plugin:catapult.plugins.streaming]

enabled = true
maxFolderNameSize = 512