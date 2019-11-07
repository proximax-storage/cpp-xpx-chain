(function prepareExchangeCollections() {
	db.createCollection('exchanges');
	db.exchanges.createIndex({ 'exchange.owner': 1 }, { unique: true });
	db.exchanges.createIndex({ 'exchange.ownerAddress': 1 }, { unique: true });
	db.exchanges.createIndex({ 'exchange.buyOffers.mosaicId': 1, 'exchange.buyOffers.price': -1 }, { unique: false });
	db.exchanges.createIndex({ 'exchange.sellOffers.mosaicId': 1, 'exchange.sellOffers.price': 1 }, { unique: false });

	db.exchanges.getIndexes();
})();
