
#include "MosaicRemoveLevyMapper.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "plugins/txes/mosaic/src/model/MosaicRemoveLevyTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {
			
	namespace {
		
		template<typename TTransaction>
		void StreamTransaction(bson_stream::document& builder, const TTransaction& transaction) {
			builder << "mosaicId" << ToInt64(transaction.MosaicId);
		}
	}
			
	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(MosaicRemoveLevy, StreamTransaction)
}}}
