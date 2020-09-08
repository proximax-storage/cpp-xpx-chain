/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "mongo/src/mappers/MapperInclude.h"
#include "plugins/txes/mosaic/src/state/LevyEntry.h"

namespace catapult { namespace mongo { namespace plugins {
			
	/// Maps a mosaic \a entry to the corresponding db model value.
	bsoncxx::document::value ToDbModel(state::LevyEntry entry);
			
	/// Maps a database \a document to the corresponding model value.
	state::LevyEntry ToLevyEntry(const bsoncxx::document::view& document);
}}}
