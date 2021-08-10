
#include "MosaicModifyLevyMapper.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "plugins/txes/mosaic/src/model/MosaicModifyLevyTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {
			
	namespace {
		
		void StreamLevy(bson_stream::array_context& context, model::MosaicLevyRaw levy)
		{
			context
				<< bson_stream::open_document
				<< "type" << utils::to_underlying_type(levy.Type)
				<< "recipient" << ToBinary(levy.Recipient)
				<< "mosaicId" << ToInt64(levy.MosaicId)
				<< "fee" << ToInt64(levy.Fee)
				<< bson_stream::close_document;
		}
		
		template<typename TTransaction>
		void StreamTransaction(bson_stream::document& builder, const TTransaction& transaction) {
			builder << "mosaicId" << ToInt64(transaction.MosaicId);
			auto levy = builder << "levy" << bson_stream::open_array;
			StreamLevy(levy, transaction.Levy);
			levy << bson_stream::close_array;
		}
	}
			
	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(MosaicModifyLevy, StreamTransaction)
}}}
