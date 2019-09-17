(function prepareContractCollections() {
	db.createCollection('files');
	db.contracts.createIndex({ 'file.key': 1 }, { unique: true });

	db.contracts.getIndexes();
})();
