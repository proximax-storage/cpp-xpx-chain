/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "mongo/src/mappers/MapperInclude.h"
#include "plugins/txes/storage/src/state/DownloadChannelEntry.h"

namespace catapult { namespace mongo { namespace plugins {

	/// Maps a drive \a entry to the corresponding db model value.
	bsoncxx::document::value ToDbModel(const state::DownloadChannelEntry& entry);

	/// Maps a database \a document to the corresponding model value.
	state::DownloadChannelEntry ToDownloadChannelEntry(const bsoncxx::document::view& document);
}}}
