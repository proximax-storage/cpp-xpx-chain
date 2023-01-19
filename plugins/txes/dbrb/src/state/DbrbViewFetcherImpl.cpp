/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DbrbViewFetcherImpl.h"
#include "src/cache/ViewSequenceCache.h"

namespace catapult { namespace state {

	dbrb::ViewData DbrbViewFetcherImpl::getLatestView() {
    	return m_pCache->sub<cache::ViewSequenceCache>().createView(m_pCache->height())->getLatestView().Data;
    }
}}
