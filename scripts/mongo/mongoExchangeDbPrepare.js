(function prepareExchangeCollections() {
	db.createCollection('exchanges');
	db.exchanges.createIndex({ 'exchange.owner': 1 }, { unique: true });

	db.exchanges.getIndexes();
})();
