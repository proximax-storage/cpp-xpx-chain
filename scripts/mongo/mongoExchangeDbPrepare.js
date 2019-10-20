(function prepareBuyOfferCollections() {
	db.createCollection('buyOffers');
	db.reputations.createIndex({ 'buyOffers.transactionHash': 1 }, { unique: true });

	db.reputations.getIndexes();
})();

(function prepareSellOfferCollections() {
	db.createCollection('sellOffers');
	db.reputations.createIndex({ 'sellOffers.transactionHash': 1 }, { unique: true });

	db.reputations.getIndexes();
})();

(function prepareDealCollections() {
	db.createCollection('deals');
	db.reputations.createIndex({ 'deals.transactionHash': 1 }, { unique: true });

	db.reputations.getIndexes();
})();

(function prepareBuyOfferCollections() {
	db.createCollection('offerDeadlines');
	db.reputations.createIndex({ 'offerDeadlines.height': 1 }, { unique: true });

	db.reputations.getIndexes();
})();
