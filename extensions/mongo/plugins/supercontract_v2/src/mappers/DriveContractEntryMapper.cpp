/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DriveContractEntryMapper.h"
#include "catapult/utils/Casting.h"
#include "mongo/src/mappers/MapperUtils.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

    // region ToDbModel

    bsoncxx::document::value ToDbModel(const state::DriveContractEntry& entry, const Address& accountAddress) {
        bson_stream::document builder;
        auto doc = builder
            << "drivecontract" << bson_stream::open_document
            << "multisig" << ToBinary(entry.key())
            << "multisigAddress" << ToBinary(accountAddress)
            << "contractKey" << ToBinary(entry.contractKey());
        
        return doc
            << bson_stream::close_document
            << bson_stream::finalize;
    }

    // endregion

    // region ToModel

    state::DriveContractEntry ToDriveContractEntry(const    bsoncxx::document::view& document) {
        auto dbDriveContractEntry = document["drivecontract"];
        Key multisig;
        DbBinaryToModelArray(multisig, dbDriveContractEntry["multisig"].get_binary());
        state::DriveContractEntry entry(multisig);

        Key contractKey;
        DbBinaryToModelArray(contractKey, dbDriveContractEntry["contractKey"].get_binary());
        entry.setContractKey(contractKey);
    }

    // endregion
}}}