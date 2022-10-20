#pragma once
#include <stdint.h>
#include "catapult/utils/BitwiseEnum.h"
namespace catapult { namespace model {

	/// Account link transaction action.
	// region state change flags
	enum class StateChangeFlags : uint64_t {
		/// No flags present.
		None = 0x00,

		/// This is the first block.
		Blockchain_Init = 0x01,

		/// Mosaic supports supply changes even when mosaic owner owns partial supply.
		Network_Config_Upgraded = 0x02,

		/// Mosaic supports transfers between arbitrary accounts.
		/// \note When not set, mosaic can only be transferred to and from mosaic owner.
		Blockchain_Upgraded = 0x04,

		/// All flags.
		All = 0x1F
	};

	MAKE_BITWISE_ENUM(StateChangeFlags);

	// endregion
}}
