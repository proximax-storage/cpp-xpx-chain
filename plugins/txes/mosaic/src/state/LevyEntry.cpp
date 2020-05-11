/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <src/catapult/exceptions.h>
#include "LevyEntry.h"

namespace catapult { namespace state {
	
	void LevyEntry::update(const LevyEntryData &levy, const Height &height) {
		m_updateHistories.emplace(height, std::move(*m_pLevy));
		m_pLevy = std::make_shared<LevyEntryData>(levy);
	}
		
	void LevyEntry::remove(const Height& height) {
		m_updateHistories.emplace(height, std::move(*m_pLevy));
		m_pLevy = nullptr;
	}
	
	void LevyEntry::undo(const Height& height) {
		
		auto iterator = m_updateHistories.find(height);
		if( iterator == m_updateHistories.end())
			CATAPULT_THROW_INVALID_ARGUMENT_1("Rollback Levy at height not exist", height);

		auto& levyAtHeight = m_updateHistories[height];
		m_pLevy = std::make_shared<LevyEntryData>(levyAtHeight);
		m_updateHistories.erase(iterator);
	}
		
	bool LevyEntry::hasUpdateHistory() {
		return m_updateHistories.size() > 0;
	}
	
}}