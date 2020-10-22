/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "MapperInclude.h"
#include "MapperUtils.h"
#include "src/catapult/model/Statement.h"

namespace catapult {
	namespace mongo { class MongoReceiptRegistry; }
}

namespace catapult { namespace mongo { namespace mappers {

	/// Maps a \a statement at \a height to the corresponding db model value using \a receiptRegistry for mapping derived receipt types.
	bsoncxx::document::value ToDbModel(
			Height height,
			const model::PublicKeyStatement& statement,
			const MongoReceiptRegistry& receiptRegistry);
}}}
