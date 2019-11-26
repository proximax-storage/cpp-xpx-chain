/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "mongo/src/mappers/MapperInclude.h"
#include "plugins/txes/exchange/src/state/ExchangeEntry.h"

namespace catapult { namespace mongo { namespace plugins {

	/// Maps an exchange \a entry and \a ownerAddress to the corresponding db model value.
	bsoncxx::document::value ToDbModel(const state::ExchangeEntry& entry, const Address& ownerAddress);

	/// Maps a database \a document to the corresponding model value.
	state::ExchangeEntry ToExchangeEntry(const bsoncxx::document::view& document);
}}}
