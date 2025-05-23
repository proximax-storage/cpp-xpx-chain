[database]

databaseUri = mongodb://{{mongo_host}}:27017
databaseName = catapult
maxWriterThreads = 8
shouldPruneFileStorage = true

[plugins]

catapult.mongo.plugins.accountlink = true
catapult.mongo.plugins.aggregate = true
catapult.mongo.plugins.committee = true
catapult.mongo.plugins.config = true
catapult.mongo.plugins.exchange = true
catapult.mongo.plugins.exchangesda = true
catapult.mongo.plugins.lockhash = true
catapult.mongo.plugins.locksecret = true
catapult.mongo.plugins.metadata = true
catapult.mongo.plugins.metadata_v2 = true
catapult.mongo.plugins.mosaic = true
catapult.mongo.plugins.multisig = true
catapult.mongo.plugins.namespace = true
catapult.mongo.plugins.property = true
catapult.mongo.plugins.transfer = true
catapult.mongo.plugins.upgrade = true
catapult.mongo.plugins.service = true
catapult.mongo.plugins.liquidityprovider = true
catapult.mongo.plugins.storage = true
catapult.mongo.plugins.operation = true
catapult.mongo.plugins.supercontract = true
catapult.mongo.plugins.streaming = true
catapult.mongo.plugins.dbrb = true
