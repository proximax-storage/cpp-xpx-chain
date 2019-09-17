(function prepareBlockchainUpgradeCollections() {
    db.createCollection('blockchainUpgrades');
    db.blockchainUpgrades.createIndex({ 'blockchainUpgrade.height': 1 }, { unique: true });

    db.blockchainUpgrades.getIndexes();
})();
