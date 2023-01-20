(function prepareDbrbCollections() {
    db.createCollection('viewSequences');
    db.viewSequences.createIndex({ 'viewSequence.hash': 1 }, { unique: true });

    db.viewSequences.getIndexes();
})();
