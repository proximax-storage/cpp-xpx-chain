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
#include "BalanceSnapshot.h"
#include "catapult/constants.h"
#include "catapult/types.h"
#include "Mosaic.h"
#include "TrailingVariableDataLayout.h"
#include <cstring>
#include <memory>

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for an account info.
	struct AccountInfo : public TrailingVariableDataLayout<AccountInfo, Mosaic> {
	public:
		/// Address of the account.
		catapult::Address Address;

		/// Height at which address has been obtained.
		Height AddressHeight;

		/// Public key of the account. Valid if PublicKeyHeight > 0.
		Key PublicKey;

		/// Height at which public key has been obtained.
		Height PublicKeyHeight;

		/// Number of types of mosaics owned by the account.
		uint16_t MosaicsCount;

		// followed by mosaics data if MosaicsCount != 0

		/// Number of types of BalanceSnapshot related to the account.
		uint16_t BalanceSnapshotCount;

		// followed by BalanceSnapshot data if BalanceHeightCount != 0

	public:
		/// Returns a const pointer to the first mosaic contained in this account info.
		const Mosaic* MosaicsPtr() const {
			return MosaicsCount ? ToTypedPointer(PayloadStart(*this)) : nullptr;
		}

		/// Returns a pointer to the first mosaic contained in this account info.
		Mosaic* MosaicsPtr() {
			return MosaicsCount ? ToTypedPointer(PayloadStart(*this)) : nullptr;
		}

		/// Returns a const pointer to the first BalanceSnapshot contained in this account info.
		const BalanceSnapshot* BalanceSnapshotPtr() const {
			return BalanceSnapshotCount ? reinterpret_cast<const BalanceSnapshot*>(PayloadStart(*this) + MosaicsCount * sizeof(Mosaic)) : nullptr;
		}

		/// Returns a pointer to the first BalanceSnapshot contained in this account info.
		BalanceSnapshot* BalanceSnapshotPtr() {
			return BalanceSnapshotCount ? reinterpret_cast<BalanceSnapshot*>(PayloadStart(*this) + MosaicsCount * sizeof(Mosaic)) : nullptr;
		}

	public:
		/// Creates a zero-initialized account info with \a address.
		static std::shared_ptr<const AccountInfo> FromAddress(const catapult::Address& address) {
			uint32_t entitySize = sizeof(AccountInfo);
			auto pAccountInfo = std::make_shared<AccountInfo>();
			std::memset(pAccountInfo.get(), 0, entitySize);
			pAccountInfo->Size = entitySize;
			pAccountInfo->Address = address;
			return pAccountInfo;
		}

	public:
		/// Calculates the real size of \a accountInfo.
		static constexpr uint64_t CalculateRealSize(const AccountInfo& accountInfo) noexcept {
			return  sizeof(AccountInfo) +
					accountInfo.MosaicsCount * sizeof(Mosaic) +
					accountInfo.BalanceSnapshotCount * sizeof(BalanceSnapshot);
		}
	};

#pragma pack(pop)

	/// Maximum size of AccountInfo containing maximum allowed number of mosaics and maximum allowed number of BalanceSnapshot.
	constexpr auto AccountInfo_Max_Size = sizeof(AccountInfo) +
			sizeof(Mosaic) * ((1 << (8 * sizeof(AccountInfo::MosaicsCount))) - 1) +
			sizeof(BalanceSnapshot) * ((1 << (8 * sizeof(AccountInfo::BalanceSnapshotCount))) - 1);
}}
