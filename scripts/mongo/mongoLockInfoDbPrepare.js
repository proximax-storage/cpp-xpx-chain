(function prepareLockInfoCollections() {
	db.createCollection('hashLockInfos');
	db.hashLockInfos.createIndex({ 'lock.hash': 1 }, { unique: true });
	db.hashLockInfos.createIndex({ 'lock.account': 1 });
	db.hashLockInfos.createIndex({ 'lock.accountAddress': 1 });

	db.createCollection('secretLockInfos');
	db.secretLockInfos.createIndex({ 'lock.compositeHash': 1 }, { unique: true });
	db.secretLockInfos.createIndex({ 'lock.account': 1 });
	db.secretLockInfos.createIndex({ 'lock.accountAddress': 1 });

	db.hashLockInfos.getIndexes();
	db.secretLockInfos.getIndexes();
})();
