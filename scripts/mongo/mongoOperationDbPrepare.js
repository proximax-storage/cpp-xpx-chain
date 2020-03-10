(function prepareOperationCollections() {
	db.createCollection('operations');
	db.operations.createIndex({ 'operation.token': 1 }, { unique: true });
	db.operations.createIndex({ 'operation.account': 1 }, { unique: false });
	db.operations.createIndex({ 'operation.accountAddress': 1 }, { unique: false });

	db.operations.getIndexes();
})();
