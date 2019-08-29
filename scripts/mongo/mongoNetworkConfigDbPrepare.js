(function prepareNetworkConfigCollections() {
    db.createCollection('networkConfigs');
    db.networkConfigs.createIndex({ 'networkConfig.height': 1 }, { unique: true });

    db.networkConfigs.getIndexes();
})();
