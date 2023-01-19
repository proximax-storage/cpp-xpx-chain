/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "catapult/cache/CatapultCache.h"
#include "catapult/dbrb/DbrbViewFetcher.h"

namespace catapult { namespace state {

    class DbrbViewFetcherImpl : public dbrb::DbrbViewFetcher {
    public:
        explicit DbrbViewFetcherImpl() = default;

    public:
		dbrb::ViewData getLatestView() override;
    };
}}
