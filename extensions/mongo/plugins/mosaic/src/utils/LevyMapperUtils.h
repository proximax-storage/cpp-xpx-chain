/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/
#pragma once
#include "mongo/src/mappers/MapperUtils.h"
#include "plugins/txes/mosaic/src/state/LevyEntry.h"

namespace catapult { namespace mongo { namespace plugins { namespace  levy {
	state::LevyEntryData ReadLevy(const bsoncxx::document::view& dbLevy);
}}}}
