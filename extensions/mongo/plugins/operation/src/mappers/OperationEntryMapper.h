/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "mongo/src/mappers/MapperInclude.h"
#include "plugins/txes/operation/src/state/OperationEntry.h"

namespace catapult { namespace mongo { namespace plugins {

	/// Maps a \a entry and \a accountAddress to the corresponding db model value.
	bsoncxx::document::value ToDbModel(const state::OperationEntry& entry, const Address& accountAddress);

	/// Maps a database \a document to \a entry.
	void ToLockInfo(const bsoncxx::document::view& document, state::OperationEntry& entry);
}}}
