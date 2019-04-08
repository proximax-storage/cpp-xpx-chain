/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "mongo/src/mappers/MapperInclude.h"
#include "plugins/txes/metadata/src/state/MetadataEntry.h"

namespace catapult { namespace mongo { namespace plugins {

	/// Maps a \a metadataEntry to the corresponding db model value.
	bsoncxx::document::value ToDbModel(const state::MetadataEntry& metadataEntry);

	/// Maps a database \a document to the corresponding model value.
	state::MetadataEntry ToMetadataEntry(const bsoncxx::document::view& document);
}}}
