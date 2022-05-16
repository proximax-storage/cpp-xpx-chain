/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "mongo/src/mappers/MapperInclude.h"
#include "plugins/txes/storage/src/state/PriorityQueueEntry.h"

namespace catapult { namespace mongo { namespace plugins {

	/// Maps a priority queue \a entry and \a key to the corresponding db model value.
	bsoncxx::document::value ToDbModel(const state::PriorityQueueEntry& entry, const Key& key);

	/// Maps a database \a document to the corresponding model value.
	state::PriorityQueueEntry ToPriorityQueueEntry(const bsoncxx::document::view& document);
}}}
