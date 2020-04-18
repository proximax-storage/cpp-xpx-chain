/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/
#pragma once
#include "catapult/types.h"

namespace catapult { namespace model {

	/// unset mosaic ID, use the current mosaic during levy
	constexpr catapult::MosaicId UnsetMosaicId (0);
	
	/// Levy fee effectivve decimal places
	constexpr uint64_t MosaicLevyFeeDecimalPlace = 100'000;
	
	/// Available mosaic levy rule ids.
	enum class LevyType : uint16_t {
		/// Default there is no levy
		None = 0x0,

		/// Constant value
		Absolute,

		/// Use percentile fee.
		Percentile,
	};

#pragma pack(push, 1)
	struct MosaicLevy {
		
		/// Levy type
		LevyType Type;

		/// Transaction recipient.
		catapult::UnresolvedAddress Recipient;

		// Levy mosaic currency
		catapult::MosaicId MosaicId;

		/// the set Levy fee
		catapult::Amount Fee;

		/// default constructor
		MosaicLevy()
			: Type(LevyType::None)
			, Recipient(catapult::UnresolvedAddress())
			, MosaicId(0)
			, Fee(Amount(0)) {
		}
		
		MosaicLevy(MosaicLevy *pLevy)
			: Type(pLevy->Type)
			, Recipient(pLevy->Recipient)
			, MosaicId(pLevy->MosaicId)
			, Fee(pLevy->Fee) {
			
		}
		/// constructor with params
		MosaicLevy(LevyType type, UnresolvedAddress recipient, catapult::MosaicId mosaicId, catapult::Amount fee)
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