(function prepareStorageCollections() {
    db.createCollection('drivesV2');
    // TODO: Should 'drive' be renamed as well?
    db.drivesV2.createIndex({ 'drive.multisig': 1 }, { unique: true });
    db.drivesV2.createIndex({ 'drive.multisigAddress': 1 }, { unique: true });
    db.drivesV2.createIndex({ 'drive.owner': 1 }, { unique: false });
    db.drivesV2.createIndex({ 'drive.rootHash': 1 }, { unique: false });
    db.drivesV2.createIndex({ 'drive.size': 1 }, { unique: false });
    db.drivesV2.createIndex({ 'drive.replicatorCount': 1 }, { unique: false });

    db.createCollection('downloadChannels');
    db.downloadChannels.createIndex({ 'downloadChannelInfo.id': 1 }, { unique: true });
    db.downloadChannels.createIndex({ 'downloadChannelInfo.consumer': 1 }, { unique: false });
    db.downloadChannels.createIndex({ 'downloadChannelInfo.driveKey': 1 }, { unique: false });
    db.downloadChannels.createIndex({ 'downloadChannelInfo.transactionFee': 1 }, { unique: false });
    db.downloadChannels.createIndex({ 'downloadChannelInfo.storageUnits': 1 }, { unique: false });

    db.createCollection('replicators');
    db.replicators.createIndex({ 'replicator.key': 1 }, { unique: true });
    db.replicators.createIndex({ 'replicator.capacity': 1 }, { unique: false });

    db.drivesV2.getIndexes();
    db.downloadChannels.getIndexes();
    db.replicators.getIndexes();
})();
