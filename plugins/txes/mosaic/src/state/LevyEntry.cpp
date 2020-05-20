/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/
#include <algorithm>
#include "LevyEntry.h"

namespace catapult { namespace state {
	
	void LevyEntry::update(const LevyEntryData &levy, const Height& height) {
		if(m_pLevy) {
			auto pair = std::make_pair(height, std::move(*m_pLevy));
			m_updateHistory.push_back(pair);
		} else {
			auto pair = std::make_pair(height, LevyEntryData());
			m_updateHistory.push_back(pair);
		}
		
		m_pLevy = std::make_shared<LevyEntryData>(levy);
	}
		
	void LevyEntry::remove(const Height& height) {
		if(!m_pLevy)
			return;
		
		auto pair = std::make_pair(height, std::move(*m_pLevy));
		m_updateHistory.push_back(pair);
		
		m_pLevy = nullptr;
	}
	
	void LevyEntry::undo() {
		auto historyPair = m_updateHistory.back();
		if( historyPair.second.Type == model::LevyType::None)
			m_pLevy = nullptr;
		else
			m_pLevy = std::make_shared<LevyEntryData>(historyPair.second);
		
		m_updateHistory.pop_back();
	}
		
	bool LevyEntry::hasUpdateHistory() {
		return m_updateHistory.size() > 0;
	}
	
	state::LevyHistoryList::iterator LevyEntry::historyAtHeight(const Height& height)
	{
		state::LevyHistoryList::iterator iterator = std::find_if(m_updateHistory.begin(), m_updateHistory.end(),
			[&height](const state::LevyHistoryPair& pair) {
				return (pair.first == height);
		});
		
		return iterator;
	}
}}