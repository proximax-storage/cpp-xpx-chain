(function prepareOfferCollections() {
	db.createCollection('offers');
	db.reputations.createIndex({ 'offer.transactionHash': 1 }, { unique: true });

	db.reputations.getIndexes();
})();
