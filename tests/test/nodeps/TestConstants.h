/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#pragma once
#include "catapult/types.h"

namespace catapult { namespace test {

	/// Default (well known) namespace id used in tests (`cat`).
	/// \note Cannot use type NamespaceId because it is defined in plugin.
	constexpr uint64_t Default_Namespace_Id(0xB149'7F5F'BA65'1B4F);

#ifdef SIGNATURE_SCHEME_NIS1
	/// Default (well known) currency mosaic id used in tests (`currency`).
	constexpr MosaicId Default_Currency_Mosaic_Id(0x44F8'F7EA'C9C6'9CEC);

	/// Default (well known) harvesting mosaic id used in tests (`harvest`).
	constexpr MosaicId Default_Harvesting_Mosaic_Id(0x44F8'F7EA'C9C6'9CEC);
#else
	/// Default (well known) currency mosaic id used in tests (`currency`).
	constexpr MosaicId Default_Currency_Mosaic_Id(0x0DC6'7FBE'1CAD'29E3);

	/// Default (well known) harvesting mosaic id used in tests (`harvest`).
	constexpr MosaicId Default_Harvesting_Mosaic_Id(0x0DC6'7FBE'1CAD'29E3);

	/// Default (well known) storage mosaic id used in tests (`storage`).
	constexpr MosaicId Default_Storage_Mosaic_Id(0x2651'4E2A'1EF3'3824);

	/// Default (well known) streaming mosaic id used in tests (`streaming`).
	constexpr MosaicId Default_Streaming_Mosaic_Id(0x6C5D'6875'08AC'9D75);

	/// Default (well known) super contract mosaic id used in tests (`supercontract`).
	constexpr MosaicId Default_Super_Contract_Mosaic_Id(0x6EE9'5526'8A1C'33D9);

	/// Default (well known) review mosaic id used in tests (`review`).
	constexpr MosaicId Default_Review_Mosaic_Id(0x19C1'CD86'7406'54DC);
#endif

	/// Default total chain importance used for scaling block target calculation.
	constexpr Importance Default_Total_Chain_Importance(8'999'999'998);

	/// Network generation hash string used by deterministic tests.
	constexpr auto Deterministic_Network_Generation_Hash_String = "070D67A92D441EAAD25AB5C78F1F68628BE33EAA1DEBEDBE14D4FBE8F4DC326E";
}}
