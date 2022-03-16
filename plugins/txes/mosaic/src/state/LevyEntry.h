/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/
#pragma once
#include <map>
#include <memory>
#include "catapult/types.h"
#include "src/model/MosaicLevy.h"

namespace catapult { namespace state {
		
	// Catapult levy entry.
	struct LevyEntryData {
		/// Levy type
		model::LevyType Type;
		
		/// Transaction recipient.
		catapult::Address Recipient;
		
		// Levy mosaic currency
		catapult::MosaicId MosaicId;
		
		/// the set Levy fee
		catapult::Amount Fee;
		
		/// default constructor
		LevyEntryData()
			: Type(model::LevyType::None)
			, Recipient(catapult::Address ())
			, MosaicId(0)
			, Fee(Amount(0)) {
		}
		
		LevyEntryData(model::LevyType type, catapult::Address recipient, catapult::MosaicId mosaicId, catapult::Amount fee)
			: Type(type)
			, Recipient(recipient)
			, MosaicId(mosaicId)
			, Fee(fee) {
			//...
		}
	};
	
	using LevyHistoryPair = std::pair<Height, LevyEntryData>;
	using LevyHistoryList = std::vector<LevyHistoryPair>;
	using LevyHistoryIterator = state::LevyHistoryList::iterator;
	
	class LevyEntry {
	public:
		LevyEntry(const MosaicId& mosaicId, const LevyEntryData& levy)
			: m_mosaicId(mosaicId)
			, m_pLevy(std::make_shared<LevyEntryData>(levy)){
		}
		
		LevyEntry(const MosaicId& mosaicId, std::shared_ptr<LevyEntryData> pLevy)
			: m_mosaicId(mosaicId)
			, m_pLevy(pLevy) {
		}
		
	public:

		void update(const LevyEntryData& levy, const Height& height);
		
		void remove(const Height& height);
		
		void undo();
		
		bool hasUpdateHistory();
		
	public:
		const MosaicId& mosaicId() const {
			return m_mosaicId;
		}
		
		const std::shared_ptr<LevyEntryData> levy() const {
			return m_pLevy;
		}
		
		/// Gets the list of update history
		LevyHistoryList& updateHistory() {
			return m_updateHistory;
		}
		
		const LevyHistoryList& updateHistory() const{
			return m_updateHistory;
		}
		
	private:
		MosaicId m_mosaicId;
		std::shared_ptr<LevyEntryData> m_pLevy;
		LevyHistoryList m_updateHistory;
	};
}}
