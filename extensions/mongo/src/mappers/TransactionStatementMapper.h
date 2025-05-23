/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
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
			const model::TransactionStatement& statement,
			const MongoReceiptRegistry& receiptRegistry);
}}}
