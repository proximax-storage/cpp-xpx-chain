/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/
#pragma once
#include "catapult/types.h"
#include <vector>
#include "src/model/MosaicLevy.h"

namespace catapult { namespace state {
		
		// Catapult upgrade entry.
		class LevyEntry {
		public:
			LevyEntry() : m_mosaicId(0)
			{}
			
			LevyEntry(MosaicId& id) : m_mosaicId(id)
			{}
			
			LevyEntry(const MosaicId& mosaicId, const model::MosaicLevy& levy)
				: m_mosaicId(mosaicId), m_levy(levy)
			{}
			
		public:
			
			const MosaicId& mosaicId() const {
				return m_mosaicId;
			}
			
			model::MosaicLevy& levyRef() {
				return m_levy;
			}
			const model::MosaicLevy& levy() const {
				return m_levy;
			}
			
		private:
			MosaicId m_mosaicId;
			model::MosaicLevy m_levy;
		};
	}}
