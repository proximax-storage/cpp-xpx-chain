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

#include "PacketIoPickerContainer.h"
#include "catapult/utils/TimeSpan.h"

namespace catapult { namespace net {

	void PacketIoPickerContainer::insert(PacketIoPicker& picker, ionet::NodeRoles roles) {
		m_pickers.emplace_back(roles, &picker);
	}

	std::vector<ionet::NodePacketIoPair> PacketIoPickerContainer::pickMatching(
			const utils::TimeSpan& ioDuration,
			ionet::NodeRoles roles) const {
		std::vector<ionet::NodePacketIoPair> ioPairs;
		for (const auto& pickerPair : m_pickers) {
			if (!HasFlag(roles, pickerPair.first))
				continue;

			auto ioPair = pickerPair.second->pickOne(ioDuration);
			if (!ioPair)
				continue;

			ioPairs.push_back(ioPair);
		}

		return ioPairs;
	}

	ionet::NodePacketIoPair PacketIoPickerContainer::pickMatching(
			const utils::TimeSpan& ioDuration,
			const Key& identityKey) const {
		for (const auto& pickerPair : m_pickers) {
			auto ioPair = pickerPair.second->pickOne(ioDuration, identityKey);
			if (ioPair)
				return ioPair;
		}

		return {};
	}

	std::vector<ionet::NodePacketIoPair> PacketIoPickerContainer::pickMultiple(const utils::TimeSpan& ioDuration) const {
		std::vector<ionet::NodePacketIoPair> ioPairs;
		for (const auto& pickerPair : m_pickers) {
			for (;;) {
				auto packetIoPair = pickerPair.second->pickOne(ioDuration);
				if (!packetIoPair)
					break;

				ioPairs.push_back(packetIoPair);
			}
		}

		return ioPairs;
	}
}}
