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
	void WriteSupplementalPublicKeys(io::OutputStream& output, const AccountPublicKeys& accountPublicKeys) {
		io::Write8(output, utils::to_underlying_type(accountPublicKeys.mask()));

		if (HasFlag(AccountPublicKeys::KeyType::Linked, accountPublicKeys.mask()))
			output.write(accountPublicKeys.linked().get());

		if (HasFlag(AccountPublicKeys::KeyType::Node, accountPublicKeys.mask()))
			output.write(accountPublicKeys.node().get());

		if (HasFlag(AccountPublicKeys::KeyType::VRF, accountPublicKeys.mask()))
			output.write(accountPublicKeys.vrf().get());
	}
	void AccountStateNonHistoricalSerializer::Save(const AccountState& accountState, io::OutputStream& output) {
		// write version
		io::Write32(output, accountState.GetVersion());
		switch(accountState.GetVersion())
		{
			case 1:
			{
				// write identifying information
				output.write(accountState.Address);
				io::Write(output, accountState.AddressHeight);
				output.write(accountState.PublicKey);
				io::Write(output, accountState.PublicKeyHeight);

				// write link information
				io::Write8(output, utils::to_underlying_type(accountState.AccountType));
				Key LinkedAccountKeyRep = HasFlag(AccountPublicKeys::KeyType::Linked, accountState.SupplementalPublicKeys.mask()) ? accountState.SupplementalPublicKeys.linked().get() : Key();
				output.write(LinkedAccountKeyRep);
				// write mosaics
				io::Write(output, accountState.Balances.optimizedMosaicId());
				io::Write(output, accountState.Balances.trackedMosaicId());
				io::Write16(output, static_cast<uint16_t>(accountState.Balances.size()));
				break;
			}
			case 2:
			{
				// write identifying information
				output.write(accountState.Address);
				io::Write(output, accountState.AddressHeight);
				output.write(accountState.PublicKey);
				io::Write(output, accountState.PublicKeyHeight);

				// write account type
				io::Write8(output, utils::to_underlying_type(accountState.AccountType));


				// write mosaics
				io::Write(output, accountState.Balances.optimizedMosaicId());
				io::Write(output, accountState.Balances.trackedMosaicId());
				io::Write16(output, static_cast<uint16_t>(accountState.Balances.size()));
				WriteSupplementalPublicKeys(output, accountState.SupplementalPublicKeys);

				auto mask = accountState.GetAdditionalDataMask();
				io::Write8(output, mask);
				if(HasAdditionalData(AdditionalDataFlags::HasOldState, mask))
				{
					Save(*accountState.OldState, output);
				}
				break;
			}
		}
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

		void ReadSupplementalPublicKey(io::InputStream& input, AccountPublicKeys::PublicKeyAccessor<Key>& publicKeyAccessor) {
			Key key;
			input.read(key);
			publicKeyAccessor.set(key);
		}


		void ReadSupplementalPublicKeys(io::InputStream& input, AccountPublicKeys& accountPublicKeys) {
			auto accountPublicKeysMask = static_cast<AccountPublicKeys::KeyType>(io::Read8(input));
			accountPublicKeys.linked().unset();
			accountPublicKeys.node().unset();
			if (HasFlag(AccountPublicKeys::KeyType::Linked, accountPublicKeysMask))
				ReadSupplementalPublicKey(input, accountPublicKeys.linked());

			if (HasFlag(AccountPublicKeys::KeyType::Node, accountPublicKeysMask))
				ReadSupplementalPublicKey(input, accountPublicKeys.node());

			if (HasFlag(AccountPublicKeys::KeyType::VRF, accountPublicKeysMask))
				ReadSupplementalPublicKey(input, accountPublicKeys.vrf());

		}
		AccountState LoadAccountStateWithoutHistory(io::InputStream& input, VersionType& version) {
			// read version
			version = io::Read32(input);
			if (version > 2)
				CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of AccountState", version);
			switch(version)
			{
				case 1:
				{
					Address address;
					input.read(address);
					auto addressHeight = io::Read<Height>(input);

					auto accountState = AccountState(address, addressHeight, 1);

					input.read(accountState.PublicKey);
					accountState.PublicKeyHeight = io::Read<Height>(input);

					// read link information
					accountState.AccountType = static_cast<state::AccountType>(io::Read8(input));
					Key tempKey;
					input.read(tempKey);
					if(tempKey != Key())
					{
						accountState.SupplementalPublicKeys.linked().unset();
						accountState.SupplementalPublicKeys.linked().set(std::move(tempKey));
					}
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
				case 2:
				{
					Address address;
					input.read(address);
					auto addressHeight = io::Read<Height>(input);

					auto accountState = AccountState(address, addressHeight, 2);

					input.read(accountState.PublicKey);
					accountState.PublicKeyHeight = io::Read<Height>(input);

					// read link information
					accountState.AccountType = static_cast<state::AccountType>(io::Read8(input));

					// read mosaics
					accountState.Balances.optimize(io::Read<MosaicId>(input));
					accountState.Balances.track(io::Read<MosaicId>(input));
					auto numMosaics = io::Read16(input);
					// read supplemental public keys
					ReadSupplementalPublicKeys(input, accountState.SupplementalPublicKeys);
					auto mask = io::Read8(input);
					if(HasAdditionalData(AdditionalDataFlags::HasOldState, mask))
					{
						VersionType _discard;
						accountState.OldState = std::make_shared<state::AccountState>(LoadAccountStateWithoutHistory(input, _discard));
					}
					for (auto i = 0u; i < numMosaics; ++i) {
						auto mosaicId = io::Read<MosaicId>(input);
						auto amount = io::Read<Amount>(input);
						accountState.Balances.credit(mosaicId, amount);
					}

					return accountState;
				}
			}
			// read identifying information

		}
	}

	AccountState AccountStateNonHistoricalSerializer::Load(io::InputStream& input) {
		VersionType version{0};
		return LoadAccountStateWithoutHistory(input, version);
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
		VersionType version{0};
		auto accountState = LoadAccountStateWithoutHistory(input, version);

		// read snapshots
		auto numSnapshots = io::Read16(input);
		for (auto i = 0u; i < numSnapshots; ++i) {
			accountState.Balances.addSnapshot(ReadBalanceSnapshot(input));
		}

		return accountState;
	}

	// endregion
}}
