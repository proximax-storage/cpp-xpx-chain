/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "OfferDeadlineEntryMapper.h"
#include "src/ExchangeMapperUtils.h"

namespace catapult { namespace mongo { namespace plugins {

	// region ToDbModel

	namespace {
		void StreamDeadline(bson_stream::array_context& context, const Timestamp& deadline, const utils::ShortHash& transactionHash) {
			context
				<< bson_stream::open_document
				<< "deadline" << ToInt64(deadline)
				<< "transactionHash" << ToInt32(transactionHash)
				<< bson_stream::close_document;
		}

#define DEFINE_STREAM_DEADLINES_FUNC(FUNC_NAME, ARRAY_NAME) \
		void FUNC_NAME(bson_stream::document &builder, const state::OfferDeadlineEntry::OfferDeadlineMap deadlines) { \
			auto offerArray = builder << #ARRAY_NAME << bson_stream::open_array; \
			for (const auto& pair : deadlines) { \
				StreamDeadline(offerArray, pair.first, pair.second); \
			} \
			offerArray << bson_stream::close_array; \
		}

		DEFINE_STREAM_DEADLINES_FUNC(StreamBuyOfferDeadlines, buyOfferDeadlines)
		DEFINE_STREAM_DEADLINES_FUNC(StreamSellOfferDeadlines, sellOfferDeadlines)

		void StreamHeight(bson_stream::array_context& context, const Height& height, const utils::ShortHash& transactionHash) {
			context
				<< bson_stream::open_document
				<< "height" << ToInt64(height)
				<< "transactionHash" << ToInt32(transactionHash)
				<< bson_stream::close_document;
		}

#define DEFINE_STREAM_HEIGHTS_FUNC(FUNC_NAME, ARRAY_NAME) \
		void FUNC_NAME(bson_stream::document &builder, const state::OfferDeadlineEntry::OfferHeightMap heights) { \
			auto offerArray = builder << #ARRAY_NAME << bson_stream::open_array; \
			for (const auto& pair : heights) { \
				StreamHeight(offerArray, pair.first, pair.second); \
			} \
			offerArray << bson_stream::close_array; \
		}

		DEFINE_STREAM_HEIGHTS_FUNC(StreamBuyOfferHeights, buyOfferHeights)
		DEFINE_STREAM_HEIGHTS_FUNC(StreamSellOfferHeights, sellOfferHeights)
	}

	bsoncxx::document::value ToDbModel(const state::OfferDeadlineEntry& entry) {
		bson_stream::document builder;
		auto doc = builder << "offerDeadline" << bson_stream::open_document
				<< "height" << ToInt64(entry.height());
		StreamBuyOfferDeadlines(builder, entry.buyOfferDeadlines());
		StreamBuyOfferHeights(builder, entry.buyOfferHeights());
		StreamSellOfferDeadlines(builder, entry.sellOfferDeadlines());
		StreamSellOfferHeights(builder, entry.sellOfferHeights());

		return doc
				<< bson_stream::close_document
				<< bson_stream::finalize;
	}

	// endregion

	// region ToModel

	namespace {
		void ReadDeadlines(const bsoncxx::array::view& dbDeadlines, state::OfferDeadlineEntry::OfferDeadlineMap& deadlines) {
			for (const auto& dbDeadline : dbDeadlines) {
				auto doc = dbDeadline.get_document().view();
				auto deadline = Timestamp{static_cast<uint64_t>(doc["deadline"].get_int64())};
				auto transactionHash = utils::ShortHash{static_cast<uint32_t>(doc["transactionHash"].get_int32())};

				deadlines.emplace(deadline, transactionHash);
			}
		}

		void ReadHeights(const bsoncxx::array::view& dbHeights, state::OfferDeadlineEntry::OfferHeightMap& heights) {
			for (const auto& dbHeight : dbHeights) {
				auto doc = dbHeight.get_document().view();
				auto height = Height{static_cast<uint64_t>(doc["height"].get_int64())};
				auto transactionHash = utils::ShortHash{static_cast<uint32_t>(doc["transactionHash"].get_int32())};

				heights.emplace(height, transactionHash);
			}
		}
	}

	state::OfferDeadlineEntry ToOfferDeadlineEntry(const bsoncxx::document::view& document) {
		auto dbOfferDeadlineEntry = document["offerDeadline"];
		auto height = Height{static_cast<uint64_t>(dbOfferDeadlineEntry["height"].get_int64())};
		state::OfferDeadlineEntry entry(height);

		ReadDeadlines(dbOfferDeadlineEntry["buyOfferDeadlines"].get_array().value, entry.buyOfferDeadlines());
		ReadHeights(dbOfferDeadlineEntry["buyOfferHeights"].get_array().value, entry.buyOfferHeights());
		ReadDeadlines(dbOfferDeadlineEntry["sellOfferDeadlines"].get_array().value, entry.sellOfferDeadlines());
		ReadHeights(dbOfferDeadlineEntry["sellOfferHeights"].get_array().value, entry.sellOfferHeights());

		return entry;
	}

	// endregion
}}}
