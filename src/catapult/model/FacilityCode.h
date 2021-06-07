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
#include <stdint.h>

namespace catapult { namespace model {

	/// Possible facility codes.
	enum class FacilityCode : uint8_t {
		/// Account link facility code.
		AccountLink = 0x4C,
		/// Aggregate facility code.
		Aggregate = 0x41,
		/// Network config facility code.
		NetworkConfig = 0x59,
		/// Contract facility code.
		Contract = 0x57,
		/// Core facility code.
		Core = 0x43,
		/// Lock hash facility code.
		LockHash = 0x48,
		/// Lock secret facility code.
		LockSecret = 0x52,
		/// Metadata facility code.
		Metadata = 0x3D,
		/// Mosaic facility code.
		Mosaic = 0x4D,
		/// Multisig facility code.
		Multisig = 0x55,
		/// Namespace facility code.
		Namespace = 0x4E,
		/// Reputation facility code.
		Reputation = 0x56,
		/// Property facility code.
		Property = 0x50,
		/// Transfer facility code.
		Transfer = 0x54,
		/// Blockchain upgrade facility code.
		BlockchainUpgrade = 0x58,
		/// Service facility code.
		Service = 0x5A,
		/// Drive facility code.
		Drive = 0x5B,
		/// Exchange facility code.
		Exchange = 0x5D,
		/// Download facility code.
		Download = 0x5E,
		/// Operation facility code.
		Operation = 0x5F,
		/// SuperContract facility code.
		SuperContract = 0x60,
		/// NEM's Metadata facility code.
		Metadata_Nem = 0x3F,
	};
}}
