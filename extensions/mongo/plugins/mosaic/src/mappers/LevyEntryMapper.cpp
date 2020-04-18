#include "mongo/src/mappers/MapperUtils.h"
#include "plugins/txes/mosaic/src/state/LevyEntry.h"
#include "catapult/utils/Casting.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {
			
	// region ToDbModel
	namespace {
		void StreamLevy(bson_stream::document& builder, const model::MosaicLevy& levy)
		{
			builder << "levy" << bson_stream::open_document
			        << "type" << utils::to_underlying_type(levy.Type)
			        << "recipient" << ToBinary(levy.Recipient)
			        << "mosaicId" << ToInt64(levy.MosaicId)
			        << "fee" << ToInt64(levy.Fee)
			        << bson_stream::close_document;
		}
		
		model::MosaicLevy ReadLevy(const bsoncxx::document::view& dbLevy) {
			model::LevyType type = static_cast<model::LevyType>(static_cast<uint32_t>(dbLevy["type"].get_int32()));
			auto mosaicId = GetValue64<MosaicId>(dbLevy["mosaicId"]);
			
			UnresolvedAddress address;
			DbBinaryToModelArray(address, dbLevy["recipient"].get_binary());
			
			Amount fee = GetValue64<Amount>(dbLevy["fee"]);
			return model::MosaicLevy(type, address, mosaicId, fee);
		}
	}
	
	bsoncxx::document::value ToDbModel(state::LevyEntry entry) {
		bson_stream::document builder;
		
		auto doc = builder
			<< "levy" << bson_stream::open_document
			<< "mosaicId" << ToInt64(entry.mosaicId());
		
		StreamLevy( builder, entry.levy());
		doc << bson_stream::close_document;
		
		return builder << bson_stream::finalize;
	}
			
	// endregion
			
	// region ToModel
	state::LevyEntry ToLevyEntry(const bsoncxx::document::view& document) {
		auto dbMosaic = document["levy"];
		auto id = GetValue64<MosaicId>(dbMosaic["mosaicId"]);
		auto levy = ReadLevy(dbMosaic["levy"].get_document().value);
		auto entry = state::LevyEntry(id, levy);
		return entry;
	}
			
	// endregion
}}}
