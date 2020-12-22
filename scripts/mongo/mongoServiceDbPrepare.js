(function prepareContractCollections() {
	db.createCollection('drives');
	db.drives.createIndex({ 'drive.multisig': 1 }, { unique: true });
	db.drives.createIndex({ 'drive.multisigAddress': 1 }, { unique: true });
	db.drives.createIndex({ 'drive.owner': 1 }, { unique: false });
	db.drives.createIndex({ 'drive.state': 1 }, { unique: false });
	db.drives.createIndex({ 'drive.replicators.replicator': 1 }, { unique: false });

	db.createCollection('downloads');
	db.downloads.createIndex({ 'downloadInfo.operationToken': 1 }, { unique: true });
	db.downloads.createIndex({ 'downloadInfo.driveKey': 1 }, { unique: false });
	db.downloads.createIndex({ 'downloadInfo.driveAddress': 1 }, { unique: false });
	db.downloads.createIndex({ 'downloadInfo.fileRecipient': 1 }, { unique: false });

	db.drives.getIndexes();
	db.downloads.getIndexes();
})();
