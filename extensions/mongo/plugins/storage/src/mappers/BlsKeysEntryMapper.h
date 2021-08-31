/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "mongo/src/mappers/MapperInclude.h"
#include "plugins/txes/storage//src/state/BlsKeysEntry.h"

namespace catapult { namespace mongo { namespace plugins {

	/// Maps a BLS keys \a entry and \a blsKey to the corresponding db model value.
	bsoncxx::document::value ToDbModel(const state::BlsKeysEntry& entry, const BLSPublicKey& blsKey);

	/// Maps a database \a document to the corresponding model value.
	state::BlsKeysEntry ToBlsKeysEntry(const bsoncxx::document::view& document);
}}}
