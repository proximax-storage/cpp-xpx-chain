/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DriveContractEntryMapper.h"
#include "mongo/src/mappers/MapperUtils.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

    // region ToDbModel

    bsoncxx::document::value ToDbModel(const state::DriveContractEntry& entry) {
        bson_stream::document builder;
        auto doc = builder
            << "drivecontract" << bson_stream::open_document
            << "driveKey" << ToBinary(entry.key())
            << "contractKey" << ToBinary(entry.contractKey());
        
        return doc
            << bson_stream::close_document
            << bson_stream::finalize;
    }

    // endregion

    // region ToModel

    state::DriveContractEntry ToDriveContractEntry(const bsoncxx::document::view& document) {
        auto dbDriveContractEntry = document["drivecontract"];
        Key driveKey;
        DbBinaryToModelArray(driveKey, dbDriveContractEntry["driveKey"].get_binary());
        state::DriveContractEntry entry(driveKey);

        Key contractKey;
        DbBinaryToModelArray(contractKey, dbDriveContractEntry["contractKey"].get_binary());
        entry.setContractKey(contractKey);

		return entry;
    }

    // endregion
}}}