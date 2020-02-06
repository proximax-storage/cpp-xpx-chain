/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "mongo/src/mappers/MapperInclude.h"
#include "plugins/txes/service/src/state/DownloadEntry.h"

namespace catapult { namespace mongo { namespace plugins {

	/// Maps a drive \a entry and \a driveAddress to the corresponding db model value.
	bsoncxx::document::value ToDbModel(const state::DownloadEntry& entry, const Address& driveAddress);

	/// Maps a database \a document to the corresponding model value.
	state::DownloadEntry ToDownloadEntry(const bsoncxx::document::view& document);
}}}
