/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/
#pragma once
#include "catapult/types.h"

namespace catapult { namespace model {
	
	/// Levy fee effectivve decimal places
	constexpr uint64_t MosaicLevyFeeDecimalPlace = 100'000;
	
	/// Available mosaic levy rule ids.
	enum class LevyType : uint8_t {
		/// Default there is no levy
		None = 0x0,

		/// Constant value
		Absolute,

		/// Use percentile fee.
		Percentile,
	};

#pragma pack(push, 1)
	struct MosaicLevyRaw {
		
		/// Levy type
		LevyType Type;

		/// Transaction recipient.
		catapult::UnresolvedAddress Recipient;

		// Levy mosaic currency
		UnresolvedMosaicId MosaicId;

		/// the set Levy fee
		catapult::Amount Fee;

		/// default constructor
		MosaicLevyRaw()
			: Type(LevyType::None)
			, Recipient(catapult::UnresolvedAddress())
			, MosaicId(0)
			, Fee(Amount(0)) {
		}
		
		MosaicLevyRaw(MosaicLevyRaw *pLevy)
			: Type(pLevy->Type)
			, Recipient(pLevy->Recipient)
			, MosaicId(pLevy->MosaicId)
			, Fee(pLevy->Fee) {
			
		}
		/// constructor with params
		MosaicLevyRaw(LevyType type, UnresolvedAddress recipient, UnresolvedMosaicId mosaicId, catapult::Amount fee)
			: Type(type)
			, Recipient(recipient)
			, MosaicId(mosaicId)
			, Fee(fee) {}
	};
#pragma pack(pop)

	struct MosaicLevyData : public UnresolvedAmountData {
	public:
		MosaicLevyData(const UnresolvedMosaicId mosaicId)
				: MosaicId(mosaicId) {}

	public:
		UnresolvedMosaicId MosaicId;
	};
}}