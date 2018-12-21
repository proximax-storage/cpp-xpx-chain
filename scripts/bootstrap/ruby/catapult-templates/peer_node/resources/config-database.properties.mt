[database]

databaseUri = mongodb://{{mongo_host}}:27017
databaseName = catapult
maxWriterThreads = 8
shouldPruneFileStorage = true

[plugins]

catapult.mongo.plugins.aggregate = true
catapult.mongo.plugins.contract = true
catapult.mongo.plugins.lockhash = true
catapult.mongo.plugins.locksecret = true
catapult.mongo.plugins.multisig = true
catapult.mongo.plugins.namespace = true
catapult.mongo.plugins.property = true
catapult.mongo.plugins.transfer = true
