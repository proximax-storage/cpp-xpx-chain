/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CommitteeManager.h"

namespace catapult { namespace chain {

	void CommitteeManager::setLastBlockElementSupplier(const model::BlockElementSupplier& supplier) {
		if (!!m_lastBlockElementSupplier)
			CATAPULT_THROW_RUNTIME_ERROR("last block element supplier already set");

		m_lastBlockElementSupplier = supplier;
	}

	const model::BlockElementSupplier& CommitteeManager::lastBlockElementSupplier() {
		if (!m_lastBlockElementSupplier)
			CATAPULT_THROW_RUNTIME_ERROR("last block element supplier not set");

		return m_lastBlockElementSupplier;
	}
}}
