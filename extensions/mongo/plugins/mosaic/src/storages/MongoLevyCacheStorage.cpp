#include "MongoLevyCacheStorage.h"
#include "src/mappers/LevyEntryMapper.h"
#include "mongo/src/storages/MongoCacheStorage.h"
#include "plugins/txes/mosaic/src/cache/LevyCache.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {
			
	namespace {
		struct LevyCacheTraits : public storages::BasicMongoCacheStorageTraits<cache::LevyCacheDescriptor> {
			static constexpr auto Collection_Name = "levy";
			static constexpr auto Id_Property_Name = "levy.mosaicId";
			
			static auto MapToMongoId(const KeyType& key) {
				return mappers::ToInt64(key);
			}
			
			static auto MapToMongoDocument(const ModelType& entry, model::NetworkIdentifier) {
				return plugins::ToDbModel(entry);
			}
		};
	}
	
	DEFINE_MONGO_FLAT_CACHE_STORAGE(Levy, LevyCacheTraits)
}}}
