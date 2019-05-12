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

#include "AccountStateSerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/utils/Casting.h"
#include "catapult/utils/MemoryUtils.h"

namespace catapult { namespace state {

	// region AccountStateNonHistoricalSerializer

	void AccountStateNonHistoricalSerializer::Save(const AccountState& accountState, io::OutputStream& output) {
        io::Write32(output, accountState.getVersion());

		// write identifying information
		io::Write(output, accountState.Address);
		io::Write(output, accountState.AddressHeight);
		io::Write(output, accountState.PublicKey);
		io::Write(output, accountState.PublicKeyHeight);

		// write link information
		io::Write8(output, utils::to_underlying_type(accountState.AccountType));
		io::Write(output, accountState.LinkedAccountKey);

		// write mosaics
		io::Write(output, accountState.Balances.optimizedMosaicId());
		io::Write(output, accountState.Balances.trackedMosaicId());
		io::Write16(output, static_cast<uint16_t>(accountState.Balances.size()));
		for (const auto& pair : accountState.Balances) {
			io::Write(output, pair.first);
			io::Write(output, pair.second);
		}
	}

	namespace {
		model::BalanceSnapshot ReadBalanceSnapshot(io::InputStream& input) {
			model::BalanceSnapshot snapshot;
			snapshot.Amount = io::Read<Amount>(input);
			snapshot.BalanceHeight = io::Read<Height>(input);
			return snapshot;
		}

		AccountState LoadAccountStateWithoutHistory(io::InputStream& input) {
            VersionType version = io::Read32(input);

			// read identifying information
			Address address;
			io::Read(input, address);
			auto addressHeight = io::Read<Height>(input);

			auto accountState = AccountState(address, addressHeight, version);

			io::Read(input, accountState.PublicKey);
			accountState.PublicKeyHeight = io::Read<Height>(input);

			// read link information
			accountState.AccountType = static_cast<state::AccountType>(io::Read8(input));
			io::Read(input, accountState.LinkedAccountKey);

			// read mosaics
			accountState.Balances.optimize(io::Read<MosaicId>(input));
			accountState.Balances.track(io::Read<MosaicId>(input));
			auto numMosaics = io::Read16(input);
			for (auto i = 0u; i < numMosaics; ++i) {
				auto mosaicId = io::Read<MosaicId>(input);
				auto amount = io::Read<Amount>(input);
				accountState.Balances.credit(mosaicId, amount);
			}

			return accountState;
		}
	}

	AccountState AccountStateNonHistoricalSerializer::Load(io::InputStream& input) {
		return LoadAccountStateWithoutHistory(input);
	}

	// endregion

	// region AccountStateSerializer

	void AccountStateSerializer::Save(const AccountState& accountState, io::OutputStream& output) {
		// write non-historical information
		AccountStateNonHistoricalSerializer::Save(accountState, output);

		// write snapshots
		io::Write16(output, static_cast<uint16_t>(accountState.Balances.snapshots().size()));
		for (const auto& pair : accountState.Balances.snapshots()) {
			io::Write(output, pair.Amount);
			io::Write(output, pair.BalanceHeight);
		}
	}

	AccountState AccountStateSerializer::Load(io::InputStream& input) {
		// read non-historical information
		auto accountState = LoadAccountStateWithoutHistory(input);

		// read snapshots
		auto numSnapshots = io::Read16(input);
		for (auto i = 0u; i < numSnapshots; ++i) {
			accountState.Balances.addSnapshot(ReadBalanceSnapshot(input));
		}

		return accountState;
	}

	// endregion
}}
