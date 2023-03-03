(function prepareServiceCollections() {
	db.createCollection('supercontracts');
	db.supercontracts.createIndex({ 'supercontract.contractKey': 1 }, { unique: true });
	db.supercontracts.createIndex({ 'supercontract.contractAddress': 1 }, { unique: true });
	db.supercontracts.createIndex({ 'supercontract.creator': 1 }, { unique: false });
	db.supercontracts.createIndex({ 'supercontract.driveKey': 1 }, { unique: false });

	db.supercontracts.getIndexes();
})();
