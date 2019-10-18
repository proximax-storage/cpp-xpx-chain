/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "mongo/src/mappers/MapperInclude.h"
#include "plugins/txes/exchange/src/state/OfferEntry.h"

namespace catapult { namespace mongo { namespace plugins {

	/// Maps a catapult upgrade \a entry to the corresponding db model value.
	bsoncxx::document::value ToDbModel(const state::OfferEntry& entry);

	/// Maps a database \a document to the corresponding model value.
	state::OfferEntry ToOfferEntry(const bsoncxx::document::view& document);
}}}
