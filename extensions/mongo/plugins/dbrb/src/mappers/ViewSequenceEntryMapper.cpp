/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ViewSequenceEntryMapper.h"
#include "catapult/utils/Casting.h"
#include "mongo/src/mappers/MapperUtils.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	// region ToDbModel

	namespace {
		void StreamView(bson_stream::document& builder, const dbrb::View& view) {
			auto array = builder << "view" << bson_stream::open_array;
			for (const auto& [processId, membershipChange] : view.Data) {
				bson_stream::document changeBuilder;
				changeBuilder
						<< "process" << ToBinary(processId)
						<< "membershipChange" << utils::to_underlying_type(membershipChange);
				array << changeBuilder;
			}
			array << bson_stream::close_array;
		}
	}

	bsoncxx::document::value ToDbModel(const state::ViewSequenceEntry& entry) {
		bson_stream::document builder;
		auto doc = builder << "viewSequence" << bson_stream::open_document
						   << "hash" << ToBinary(entry.hash());

		auto array = builder << "sequence" << bson_stream::open_array;
		for (const auto& view : entry.sequence().data()) {
			StreamView(builder, view);
		}
		array << bson_stream::close_array;

		return doc
				<< bson_stream::close_document
				<< bson_stream::finalize;
	}

	// endregion

	// region ToModel

	namespace {
		void ReadView(dbrb::View& view, const bsoncxx::array::view& dbChangesMap) {
			auto& viewData = view.Data;
			viewData.clear();
			for (const auto& dbChange : dbChangesMap) {
				auto doc = dbChange.get_document().view();

				Key processId;
				DbBinaryToModelArray(processId, doc["process"].get_binary());
				const auto membershipChange = static_cast<dbrb::MembershipChange>(static_cast<uint8_t>(doc["membershipChange"].get_int32()));

				viewData.emplace(std::move(processId), membershipChange);
			}
		}
	}

	state::ViewSequenceEntry ToViewSequenceEntry(const bsoncxx::document::view& document) {
		auto dbViewSequenceEntry = document["viewSequence"];
		Hash256 hash;
		DbBinaryToModelArray(hash, dbViewSequenceEntry["hash"].get_binary());
		state::ViewSequenceEntry entry(hash);

		for (const auto& dbView : dbViewSequenceEntry["sequence"].get_array().value) {
			dbrb::View view;
			ReadView(view, dbView["view"].get_array().value);
			entry.sequence().tryAppend(view);
		}

		return entry;
	}

	// endregion
}}}
