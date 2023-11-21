(function prepareDbrbCollections() {
    db.createCollection('dbrbProcesses');
    db.dbrbProcesses.createIndex({ 'dbrbProcess.processId': 1 }, { unique: true });

    db.dbrbProcesses.getIndexes();
})();
