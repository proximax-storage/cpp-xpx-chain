#pragma once

namespace catapult { namespace model {
	/// Available mosaic levy rule ids.
	enum class LevyType : uint16_t {
		/// Default there is no levy
		None = 0x1,

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
		UnresolvedAddress Recipient;

		// Levy mosaic currency
		catapult::MosaicId MosaicId;

		/// the set Levy fee
		catapult::Amount Fee;

		/// default constructor
		MosaicLevy() = default;

		/// constructor with params
		MosaicLevy(LevyType type, UnresolvedAddress recipient, catapult::MosaicId mosaicId, catapult::Amount fee)
			: Type(type)
			, Recipient(recipient)
			, MosaicId(mosaicId)
			, Fee(fee) {}
	};
#pragma pack(pop)
}}