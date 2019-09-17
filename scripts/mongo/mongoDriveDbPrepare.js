(function prepareContractCollections() {
	db.createCollection('drives');
	db.contracts.createIndex({ 'drive.multisig': 1 }, { unique: true });
	db.contracts.createIndex({ 'drive.multisigAddress': 1 }, { unique: true });

	db.contracts.getIndexes();
})();
