/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "mongo/src/mappers/MapperInclude.h"
#include "plugins/txes/exchange_sda/src/state/SdaOfferGroupEntry.h"

namespace catapult { namespace mongo { namespace plugins {

    /// Maps an SDA-SDA offer group \a entry to the corresponding db model value.
    bsoncxx::document::value ToDbModel(const state::SdaOfferGroupEntry& entry);

    /// Maps a database \a document to the corresponding model value.
    state::SdaOfferGroupEntry ToSdaOfferGroupEntry(const bsoncxx::document::view& document);
}}}