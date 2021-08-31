(function prepareStorageCollections() {
    db.createCollection('bcdrives');
    db.bcdrives.createIndex({ 'drive.multisig': 1 }, { unique: true });
    db.bcdrives.createIndex({ 'drive.multisigAddress': 1 }, { unique: true });

    db.createCollection('downloadChannels');
    db.downloadChannels.createIndex({ 'downloadChannelInfo.id': 1 }, { unique: true });

    db.createCollection('replicators');
    db.replicators.createIndex({ 'replicator.key': 1 }, { unique: true });

    db.createCollection('blsKeys');
    db.blsKeys.createIndex({ 'blsKeyDoc.blsKey': 1 }, { unique: true });

    db.bcdrives.getIndexes();
    db.downloadChannels.getIndexes();
    db.replicators.getIndexes();
    db.blsKeys.getIndexes();
})();
