/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "mongo/src/mappers/MapperInclude.h"
#include "plugins/txes/upgrade/src/state/BlockchainUpgradeEntry.h"

namespace catapult { namespace mongo { namespace plugins {

	/// Maps a catapult upgrade \a entry to the corresponding db model value.
	bsoncxx::document::value ToDbModel(const state::BlockchainUpgradeEntry& entry);

	/// Maps a database \a document to the corresponding model value.
	state::BlockchainUpgradeEntry ToBlockchainUpgradeEntry(const bsoncxx::document::view& document);
}}}
