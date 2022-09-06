/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "LiquidityProviderEntryMapper.h"
#include "catapult/utils/Casting.h"
#include "mongo/src/mappers/MapperUtils.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	// region ToDbModel

	namespace {

		void StreamRecentTurnover(bson_stream::document& builder, const state::HistoryObservation& observation) {
			const auto& rate = observation.m_rate;
			builder << "recentTurnover" << bson_stream::open_document
			        << "rate" << bson_stream::open_document
					<< "currencyAmount" << static_cast<int64_t>(rate.m_currencyAmount.unwrap())
					<< "mosaicAmount" << static_cast<int64_t>(rate.m_mosaicAmount.unwrap())
					<< bson_stream::close_document
					<< "turnover" << static_cast<int64_t>(observation.m_turnover.unwrap())
					<< bson_stream::close_document;
		}

		void StreamHistoryObservations(bson_stream::document& builder, const std::deque<state::HistoryObservation>& observations) {
			auto array = builder << "turnoverHistory" << bson_stream::open_array;
			for (const auto& observation: observations) {
				const auto& rate = observation.m_rate;
				array << bson_stream::open_document
					  << "rate" << bson_stream::open_document
					  << "currencyAmount" << static_cast<int64_t>(rate.m_currencyAmount.unwrap())
					  << "mosaicAmount" << static_cast<int64_t>(rate.m_mosaicAmount.unwrap())
					  << bson_stream::close_document
					  << "turnover" << static_cast<int64_t>(observation.m_turnover.unwrap())
					  << bson_stream::close_document;
			}
			array << bson_stream::close_array;
		}
	}

	bsoncxx::document::value ToDbModel(const state::LiquidityProviderEntry& entry) {
		bson_stream::document builder;
		auto doc = builder << "liquidityProvider" << bson_stream::open_document
				           << "mosaicId" << static_cast<int64_t>(entry.mosaicId().unwrap())
				           << "providerKey" << ToBinary(entry.providerKey())
				           << "owner" << ToBinary(entry.owner())
				           << "additionallyMinted" << static_cast<int64_t>(entry.additionallyMinted().unwrap())
				           << "slashingAccount" << ToBinary(entry.slashingAccount())
				           << "slashingPeriod" << static_cast<int32_t>(entry.slashingPeriod())
				           << "windowSize" << static_cast<int32_t>(entry.windowSize())
				           << "creationHeight" << static_cast<int64_t>(entry.creationHeight().unwrap())
				           << "alpha" << static_cast<int32_t>(entry.alpha())
				   		   << "beta" << static_cast<int32_t>(entry.beta());

		StreamHistoryObservations(builder, entry.turnoverHistory());
		StreamRecentTurnover(builder, entry.recentTurnover());

		return doc
			   << bson_stream::close_document
			   << bson_stream::finalize;
	}

	// endregion

	// region ToModel

	namespace {

		void ReadHistoryObservation(state::HistoryObservation& historyObservation, const bsoncxx::document::view& dbHistoryObservation) {
			auto exchangeRate = dbHistoryObservation["rate"].get_document().view();
			historyObservation.m_rate.m_currencyAmount = Amount{static_cast<uint64_t>(exchangeRate["currencyAmount"].get_int64())};
			historyObservation.m_rate.m_mosaicAmount = Amount{static_cast<uint64_t>(exchangeRate["mosaicAmount"].get_int64())};
			historyObservation.m_turnover = Amount{static_cast<uint64_t>(dbHistoryObservation["turnover"].get_int64())};
		}

		void ReadHistoryObservations(std::deque<state::HistoryObservation>& historyObservations, const bsoncxx::array::view& dbHistoryObservations) {
			for (const auto& dbObservation : dbHistoryObservations) {
				auto document = dbObservation.get_document().view();
				state::HistoryObservation observation;
				ReadHistoryObservation(observation, document);
			}
		}

	}

	state::LiquidityProviderEntry ToLiquidityProviderEntry(const bsoncxx::document::view& document) {

		auto liquidityProviderEntry = document["liquidityProvider"];

		auto mosaicId = UnresolvedMosaicId{static_cast<uint64_t>(liquidityProviderEntry["mosaicId"].get_int64())};
		state::LiquidityProviderEntry entry(mosaicId);

		Key providerKey;
		DbBinaryToModelArray(providerKey, liquidityProviderEntry["providerKey"].get_binary());
		entry.setProviderKey(providerKey);

		Key owner;
		DbBinaryToModelArray(owner, liquidityProviderEntry["owner"].get_binary());
		entry.setOwner(owner);

		entry.setAdditionallyMinted(Amount{static_cast<uint64_t>(liquidityProviderEntry["additionallyMinted"].get_int64())});

		Key slashingAccount;
		DbBinaryToModelArray(slashingAccount, liquidityProviderEntry["slashingAccount"].get_binary());
		entry.setSlashingAccount(slashingAccount);

		entry.setSlashingPeriod(static_cast<uint32_t>(liquidityProviderEntry["slashingPeriod"].get_int32()));

		entry.setWindowSize(static_cast<uint16_t>(liquidityProviderEntry["windowSize"].get_int32()));

		entry.setCreationHeight(Height{static_cast<uint64_t>(liquidityProviderEntry["creationHeight"].get_int64())});

		entry.setAlpha(static_cast<uint32_t>(liquidityProviderEntry["alpha"].get_int32()));

		entry.setBeta(static_cast<uint32_t>(liquidityProviderEntry["beta"].get_int32()));

		ReadHistoryObservations(entry.turnoverHistory(), liquidityProviderEntry["turnoverHistory"].get_array().value);

		ReadHistoryObservation(entry.recentTurnover(), liquidityProviderEntry["recentTurnover"].get_document().view());
		return entry;
	}

	// endregion
}}}
