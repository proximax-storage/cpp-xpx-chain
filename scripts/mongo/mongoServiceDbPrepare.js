(function prepareContractCollections() {
	db.createCollection('drives');
	db.contracts.createIndex({ 'drive.multisig': 1 }, { unique: true });
	db.contracts.createIndex({ 'drive.multisigAddress': 1 }, { unique: true });
	db.contracts.createIndex({ 'drive.owner': 1 }, { unique: false });
	db.contracts.createIndex({ 'drive.replicators.replicator': 1 }, { unique: false });

	db.contracts.getIndexes();
})();
