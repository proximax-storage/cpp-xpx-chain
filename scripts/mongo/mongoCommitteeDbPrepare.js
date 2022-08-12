(function prepareCommitteeCollections() {
	db.createCollection('harvesters');
	db.harvesters.createIndex({ 'harvester.key': 1 }, { unique: true });
	db.harvesters.createIndex({ 'harvester.address': 1 }, { unique: true });

	db.harvesters.getIndexes();
})();
