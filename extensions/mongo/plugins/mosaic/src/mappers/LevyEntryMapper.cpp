#include "mongo/src/mappers/MapperUtils.h"
#include "plugins/txes/mosaic/src/state/LevyEntry.h"
#include "catapult/utils/Casting.h"
#include "src/utils/LevyMapperUtils.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {
	
	// region ToDbModel
	namespace {
		void StreamLevy(bson_stream::document& builder, const state::LevyEntryData& levy) {
			builder << "type" << utils::to_underlying_type(levy.Type)
			        << "recipient" << ToBinary(levy.Recipient)
			        << "mosaicId" << ToInt64(levy.MosaicId)
			        << "fee" << ToInt64(levy.Fee);
		}
		
		void StreamHistory(bson_stream::document& builder, const state::LevyHistoryMap& historyMap) {
			auto historyInfo = builder << "history" << bson_stream::open_array;
			for (const auto& history : historyMap) {
				bson_stream::document levyHistoryBuilder;
				levyHistoryBuilder << "height" << ToInt64(history.first);
				StreamLevy(levyHistoryBuilder, history.second);
				historyInfo << levyHistoryBuilder;
			}
			
			historyInfo << bson_stream::close_array;
		}
		
		void ReadHistory(const bsoncxx::array::view& historyEntry, state::LevyHistoryMap& historyMap) {
			for (const auto& dbHistoryItem : historyEntry) {
				auto doc = dbHistoryItem.get_document().view();
				auto height = Height{static_cast<uint64_t>(doc["height"].get_int64())};
				state::LevyEntryData entry = levy::ReadLevy(doc);
				historyMap.emplace(height, entry);
			}
		}
	}
	
	bsoncxx::document::value ToDbModel(state::LevyEntry entry) {
		bson_stream::document builder;
		
		auto levy = entry.levy();
		auto doc = builder
			<< "levy" << bson_stream::open_document
			<< "mosaicId" << ToInt64(entry.mosaicId())
			<< "flag" << static_cast<int32_t>(levy ? 1 : 0);
		
		if(levy) {
			builder << "levy" << bson_stream::open_document;
			StreamLevy(builder, *levy);
			builder << bson_stream::close_document;
		}
		
		StreamHistory(builder, entry.updateHistories());
		
		doc << bson_stream::close_document;
		
		return builder << bson_stream::finalize;
	}
			
	// endregion
			
	// region ToModel
	
	state::LevyEntry ToLevyEntry(const bsoncxx::document::view& document) {
		auto dbMosaic = document["levy"];
		auto id = GetValue64<MosaicId>(dbMosaic["mosaicId"]);
		auto flag = ToUint8(dbMosaic["flag"].get_int32());
		
		std::shared_ptr<state::LevyEntryData> pLevy = nullptr;
		if( flag ) {
			auto levy = levy::ReadLevy(dbMosaic["levy"].get_document().value);
			pLevy = std::make_shared<state::LevyEntryData>(levy);
		}
		
		auto entry = state::LevyEntry(id, pLevy);
		
		// load histories
		ReadHistory(dbMosaic["history"].get_array().value, entry.updateHistories());
		
		return entry;
	}
	
	// endregion
}}}
