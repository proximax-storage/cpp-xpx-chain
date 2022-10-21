(function prepareSdaExchangeCollections() {
    db.createCollection('exchangesda');
    db.exchangesda.createIndex({ 'exchangesda.owner': 1 }, { unique: true });
    db.exchangesda.createIndex({ 'exchangesda.ownerAddress': 1 }, { unique: true });
    db.exchangesda.createIndex({ 'exchangesda.sdaOfferBalances.mosaicIdGive': 1 }, { unique: false });
    db.exchangesda.createIndex({ 'exchangesda.sdaOfferBalances.mosaicIdGet': 1 }, { unique: false });

    db.exchangesda.getIndexes();
})();
