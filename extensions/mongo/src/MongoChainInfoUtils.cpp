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

#include "MongoChainInfoUtils.h"
#include "MongoBulkWriter.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo {

	BulkWriteResult TrySetChainInfoDocument(mongocxx::database& database, const bsoncxx::document::view& upsertDoc) {
		auto collection = database["chainInfo"];
		auto result = collection.update_one({}, upsertDoc, mongocxx::options::update().upsert(true)).value().result();
		return BulkWriteResult(result);
	}

	bsoncxx::document::value GetChainInfoDocument(const mongocxx::database& database) {
		auto collection = database["chainInfo"];
		auto matchedDocument = collection.find_one({});
		if (matchedDocument.has_value())
			return matchedDocument.value();

		return bsoncxx::document::value(nullptr, 0, [](auto*) {});
	}
}}
