(function prepareExchangeCollections() {
	db.createCollection('exchanges');
	db.reputations.createIndex({ 'exchange.owner': 1 }, { unique: true });

	db.reputations.getIndexes();
})();
