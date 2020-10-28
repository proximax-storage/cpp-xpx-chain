#pragma once
#include "mongo/src/mappers/MapperInclude.h"
#include "plugins/txes/mosaic/src/state/LevyEntry.h"

namespace catapult { namespace mongo { namespace plugins {
			
	/// Maps a levy \a entry to the corresponding db model value.
	bsoncxx::document::value ToDbModel(state::LevyEntry entry);
			
	/// Maps a database \a document to the corresponding model value.
	state::LevyEntry ToLevyEntry(const bsoncxx::document::view& document);
}}}
