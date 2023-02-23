/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "mongo/src/mappers/MapperInclude.h"
#include "plugins/txes/supercontract_v2/src/state/DriveContractEntry.h"

namespace catapult { namespace mongo { namespace plugins {

	/// Maps a drive contract \a entry and \a accountAddress to the corresponding db model value.
	bsoncxx::document::value ToDbModel(const state::DriveContractEntry& entry, const Address& accountAddress);

	/// Maps a database \a document to the corresponding model value.
	state::DriveContractEntry ToDriveContractEntry(const bsoncxx::document::view& document);
}}}
