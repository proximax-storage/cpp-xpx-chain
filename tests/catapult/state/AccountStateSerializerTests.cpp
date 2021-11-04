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

#include "catapult/state/AccountStateSerializer.h"
#include "catapult/model/Mosaic.h"
#include "tests/test/core/AccountStateTestUtils.h"
#include "tests/test/core/AddressTestUtils.h"
#include "tests/test/core/mocks/MockMemoryStream.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS AccountStateSerializerTests

	namespace {
		size_t GetManyMosaicsCount() {
			return test::GetStressIterationCount() ? 65535 : 1000;
		}
	}

	// region raw serialization

#ifdef _MSC_VER
#define MAY_ALIAS
#else
#define MAY_ALIAS __attribute__((may_alias))
#endif

	namespace {
#pragma pack(push, 1)

		template<uint32_t TVersion>
		struct AccountStateHeader{};

		template<>
		struct AccountStateHeader<1> {
			catapult::Address Address;
			Height AddressHeight;
			Key PublicKey;
			Height PublicKeyHeight;

			state::AccountType AccountType;
			Key LinkedAccountKey;

			MosaicId OptimizedMosaicId;
			MosaicId TrackedMosaicId;
			uint16_t MosaicsCount;
		};
		template<>
		struct AccountStateHeader<2> {
			catapult::Address Address;
			Height AddressHeight;
			Key PublicKey;
			Height PublicKeyHeight;
			state::AccountType AccountType;
			MosaicId OptimizedMosaicId;
			MosaicId TrackedMosaicId;
			uint16_t MosaicsCount;
			AccountPublicKeys::KeyType LinkedKeysMask;
		};
		struct MAY_ALIAS HistoricalSnapshotsHeader {
			uint16_t SnapshotsCount;
		};

#pragma pack(pop)

		const Key* GetSupplementalKeysPointer(const AccountStateHeader<2>& header)
		{
			return reinterpret_cast<const Key*>(&header + 1);
		}

		size_t SetPublicKeyFromDataToBuffer(const AccountPublicKeys::PublicKeyAccessor<Key>& publicKeyAccessor, uint8_t* pData) {
			auto tData = reinterpret_cast<Key*>(pData);
			*tData = publicKeyAccessor.get();
			return Key::Size;
		}


		AccountState CreateRandomAccountState(size_t numMosaics, uint32_t version) {
			auto accountState = AccountState(test::GenerateRandomAddress(), Height(123), version);
			test::FillWithRandomData(accountState.PublicKey);
			accountState.PublicKeyHeight = Height(234);

			accountState.AccountType = static_cast<AccountType>(33);
			accountState.SupplementalPublicKeys.linked().unset();
			accountState.SupplementalPublicKeys.node().unset();
			Key temp;
			test::FillWithRandomData(temp);
			accountState.SupplementalPublicKeys.linked().set(std::move(temp));
			test::FillWithRandomData(temp);
			accountState.SupplementalPublicKeys.node().set(std::move(temp));
			test::FillWithRandomData(temp);
			accountState.SupplementalPublicKeys.vrf().set(std::move(temp));
			test::RandomFillAccountData(0, accountState, numMosaics, numMosaics);
			accountState.Balances.optimize(test::GenerateRandomValue<MosaicId>());
			return accountState;
		}

		template<typename TTraits, uint32_t TVersion>
		struct VersionedStateSerializerUtils {};


		template<typename TTraits>
		struct VersionedStateSerializerUtils<TTraits, 1> {
			static size_t CalculatePackedSize(const AccountState& accountState) {
				return sizeof(VersionType) + sizeof(AccountStateHeader<1>) + sizeof(HistoricalSnapshotsHeader)
					   + accountState.Balances.size() * sizeof(model::Mosaic)
					   + accountState.Balances.snapshots().size() * sizeof(model::BalanceSnapshot);
			}
			static const model::Mosaic* GetMosaicPointer(const AccountStateHeader<1>& header) {
				const auto* pHeaderData = reinterpret_cast<const uint8_t*>(&header + 1);
				return reinterpret_cast<const model::Mosaic*>(pHeaderData);
			}
			static AccountState CopyHeaderToAccountState(const AccountStateHeader<1>& header) {
				auto accountState = AccountState(header.Address, header.AddressHeight, 1);
				accountState.PublicKey = header.PublicKey;
				accountState.PublicKeyHeight = header.PublicKeyHeight;

				accountState.AccountType = header.AccountType;
				accountState.SupplementalPublicKeys.linked().unset();
				accountState.SupplementalPublicKeys.linked().set(header.LinkedAccountKey);

				accountState.Balances.optimize(header.OptimizedMosaicId);
				accountState.Balances.track(header.TrackedMosaicId);
				const auto* pMosaic = GetMosaicPointer(header);
				for (auto i = 0u; i < header.MosaicsCount; ++i, ++pMosaic)
					accountState.Balances.credit(pMosaic->MosaicId, pMosaic->Amount);

				if (TTraits::Has_Historical_Snapshots) {
					const auto& historicalSnapshotsHeader = reinterpret_cast<const HistoricalSnapshotsHeader&>(*pMosaic);
					const auto* pHeaderData = reinterpret_cast<const uint8_t*>(&historicalSnapshotsHeader + 1);

					const auto* pBalanceSnapshot = reinterpret_cast<const model::BalanceSnapshot*>(pHeaderData);
					for (int i = 0; i < historicalSnapshotsHeader.SnapshotsCount; ++i, ++pBalanceSnapshot)
						accountState.Balances.addSnapshot(*pBalanceSnapshot);
				}

				return accountState;
			}
			static std::vector<uint8_t> CopyToBuffer(const AccountState& accountState) {
				std::vector<uint8_t> buffer(CalculatePackedSize(accountState));

				AccountStateHeader<1> header;
				header.Address = accountState.Address;
				header.AddressHeight = accountState.AddressHeight;
				header.PublicKey = accountState.PublicKey;
				header.PublicKeyHeight = accountState.PublicKeyHeight;

				header.AccountType = accountState.AccountType;
				header.LinkedAccountKey = accountState.SupplementalPublicKeys.linked().get();

				header.OptimizedMosaicId = accountState.Balances.optimizedMosaicId();
				header.TrackedMosaicId = accountState.Balances.trackedMosaicId();
				header.MosaicsCount = static_cast<uint16_t>(accountState.Balances.size());

				auto* pData = buffer.data();

				VersionType version{1};
				std::memcpy(pData, &version, sizeof(VersionType));
				pData += sizeof(VersionType);

				std::memcpy(pData, &header, sizeof(AccountStateHeader<1>));
				pData += sizeof(AccountStateHeader<1>);

				auto* pUint64Data = reinterpret_cast<uint64_t*>(pData);
				for (const auto& pair : accountState.Balances) {
					*pUint64Data++ = pair.first.unwrap();
					*pUint64Data++ = pair.second.unwrap();
				}

				pData = reinterpret_cast<uint8_t*>(pUint64Data);
				HistoricalSnapshotsHeader historicalHeader;
				historicalHeader.SnapshotsCount = static_cast<uint16_t>(accountState.Balances.snapshots().size());
				std::memcpy(pData, &historicalHeader, sizeof(HistoricalSnapshotsHeader));
				pData += sizeof(HistoricalSnapshotsHeader);

				pUint64Data = reinterpret_cast<uint64_t*>(pData);
				for (const auto& snapshot : accountState.Balances.snapshots()) {
					*pUint64Data++ = snapshot.Amount.unwrap();
					*pUint64Data++ = snapshot.BalanceHeight.unwrap();
				}

				return buffer;
			}
		};

		template<typename TTraits>
		struct VersionedStateSerializerUtils<TTraits, 2> {
			static size_t CalculatePackedSize(const AccountState& accountState) {
				auto size = sizeof(VersionType) + sizeof(AccountStateHeader<2>) + sizeof(HistoricalSnapshotsHeader)
					   + (HasFlag(AccountPublicKeys::KeyType::Linked, accountState.SupplementalPublicKeys.mask()) ? Key::Size : 0)
					   + (HasFlag(AccountPublicKeys::KeyType::Node, accountState.SupplementalPublicKeys.mask()) ? Key::Size : 0)
					   + (HasFlag(AccountPublicKeys::KeyType::VRF, accountState.SupplementalPublicKeys.mask()) ? Key::Size : 0)
					   + 8
					   + accountState.Balances.size() * sizeof(model::Mosaic)
					   + accountState.Balances.snapshots().size() * sizeof(model::BalanceSnapshot);
				if(HasAdditionalData(AdditionalDataFlags::HasOldState, accountState.GetAdditionalDataMask()))
				{
					if(accountState.OldState->GetVersion() == 1)
						size += VersionedStateSerializerUtils<TTraits, 1>::CalculatePackedSize(*accountState.OldState);
					else if(accountState.OldState->GetVersion() == 2)
						size += VersionedStateSerializerUtils<TTraits, 2>::CalculatePackedSize(*accountState.OldState);
				}
				return size;
			}
			static const model::Mosaic* GetMosaicPointer(const AccountStateHeader<2>& header) {
				const auto* pHeaderData = reinterpret_cast<const uint8_t*>(&header + 1)
										  + (HasFlag(AccountPublicKeys::KeyType::Linked, header.LinkedKeysMask) ? Key::Size : 0)
										  + (HasFlag(AccountPublicKeys::KeyType::VRF, header.LinkedKeysMask) ? Key::Size : 0)
										  + (HasFlag(AccountPublicKeys::KeyType::Node, header.LinkedKeysMask) ? Key::Size : 0);
				return reinterpret_cast<const model::Mosaic*>(pHeaderData);
			}
			static AccountState CopyHeaderToAccountState(const AccountStateHeader<2>& header) {
				auto accountState = AccountState(header.Address, header.AddressHeight, 2);
				accountState.PublicKey = header.PublicKey;
				accountState.PublicKeyHeight = header.PublicKeyHeight;

				accountState.AccountType = header.AccountType;

				accountState.Balances.optimize(header.OptimizedMosaicId);
				accountState.Balances.track(header.TrackedMosaicId);
				accountState.SupplementalPublicKeys.linked().unset();
				accountState.SupplementalPublicKeys.node().unset();
				accountState.SupplementalPublicKeys.vrf().unset();
				auto* pKeysPointer = GetSupplementalKeysPointer(header);
				if(HasFlag(AccountPublicKeys::KeyType::Linked, header.LinkedKeysMask))
				{
					accountState.SupplementalPublicKeys.linked().set(*pKeysPointer);
					pKeysPointer++;
				}
				if(HasFlag(AccountPublicKeys::KeyType::Node, header.LinkedKeysMask))
				{
					accountState.SupplementalPublicKeys.node().set(*pKeysPointer);
					pKeysPointer++;
				}
				if(HasFlag(AccountPublicKeys::KeyType::VRF, header.LinkedKeysMask))
				{
					accountState.SupplementalPublicKeys.vrf().set(*pKeysPointer);
					pKeysPointer++;
				}
				auto bytePointer = reinterpret_cast<const uint8_t*>(pKeysPointer);
				auto mask = *bytePointer;
				int forwardMove;
				if(HasAdditionalData(AdditionalDataFlags::HasOldState, mask))
				{
					auto accountState = CopyHeaderToAccountState(*reinterpret_cast<const AccountStateHeader<2>*>(bytePointer));
					if(accountState.OldState->GetVersion() == 1)
						forwardMove = VersionedStateSerializerUtils<TTraits, 1>::CalculatePackedSize(*accountState.OldState);
					else if(accountState.OldState->GetVersion() == 2)
						forwardMove = VersionedStateSerializerUtils<TTraits, 2>::CalculatePackedSize(*accountState.OldState);
				}
				accountState.Balances.optimize(header.OptimizedMosaicId);
				accountState.Balances.track(header.TrackedMosaicId);

				const auto* pMosaicExpectedLocation = reinterpret_cast<const uint8_t*>(GetMosaicPointer(header));
				pMosaicExpectedLocation+=forwardMove;
				const auto* pMosaic = reinterpret_cast<const model::Mosaic*>(pMosaicExpectedLocation);
				for (auto i = 0u; i < header.MosaicsCount; ++i, ++pMosaic)
					accountState.Balances.credit(pMosaic->MosaicId, pMosaic->Amount);

				if (TTraits::Has_Historical_Snapshots) {
					const auto& historicalSnapshotsHeader = reinterpret_cast<const HistoricalSnapshotsHeader&>(*pMosaic);
					const auto* pHeaderData = reinterpret_cast<const uint8_t*>(&historicalSnapshotsHeader + 1);

					const auto* pBalanceSnapshot = reinterpret_cast<const model::BalanceSnapshot*>(pHeaderData);
					for (int i = 0; i < historicalSnapshotsHeader.SnapshotsCount; ++i, ++pBalanceSnapshot)
						accountState.Balances.addSnapshot(*pBalanceSnapshot);
				}

				return accountState;
			}
			static std::vector<uint8_t> CopyToBuffer(const AccountState& accountState) {
				std::vector<uint8_t> buffer(CalculatePackedSize(accountState));

				AccountStateHeader<2> header;
				header.Address = accountState.Address;
				header.AddressHeight = accountState.AddressHeight;
				header.PublicKey = accountState.PublicKey;
				header.PublicKeyHeight = accountState.PublicKeyHeight;

				header.AccountType = accountState.AccountType;
				header.LinkedKeysMask = accountState.SupplementalPublicKeys.mask();
				header.OptimizedMosaicId = accountState.Balances.optimizedMosaicId();
				header.TrackedMosaicId = accountState.Balances.trackedMosaicId();
				header.MosaicsCount = static_cast<uint16_t>(accountState.Balances.size());

				auto* pData = buffer.data();

				VersionType version{2};
				std::memcpy(pData, &version, sizeof(VersionType));
				pData += sizeof(VersionType);
				std::memcpy(pData, &header, sizeof(AccountStateHeader<2>));
				pData += sizeof(AccountStateHeader<2>);
				auto mask = accountState.GetAdditionalDataMask();
				*pData = mask;
				pData++;
				if(HasAdditionalData(AdditionalDataFlags::HasOldState, accountState.GetAdditionalDataMask()))
				{
					auto data = CopyToBuffer(*accountState.OldState);
					std::memcpy(pData, data.data(), sizeof(data));
					pData += sizeof(data);
				}
				if (HasFlag(AccountPublicKeys::KeyType::Linked, header.LinkedKeysMask))
					pData += SetPublicKeyFromDataToBuffer(accountState.SupplementalPublicKeys.linked(), pData);

				if (HasFlag(AccountPublicKeys::KeyType::Node, header.LinkedKeysMask))
					pData += SetPublicKeyFromDataToBuffer(accountState.SupplementalPublicKeys.node(), pData);

				if (HasFlag(AccountPublicKeys::KeyType::VRF, header.LinkedKeysMask))
					pData += SetPublicKeyFromDataToBuffer(accountState.SupplementalPublicKeys.vrf(), pData);

				auto* pUint64Data = reinterpret_cast<uint64_t*>(pData);
				for (const auto& pair : accountState.Balances) {
					*pUint64Data++ = pair.first.unwrap();
					*pUint64Data++ = pair.second.unwrap();
				}

				pData = reinterpret_cast<uint8_t*>(pUint64Data);
				HistoricalSnapshotsHeader historicalHeader;
				historicalHeader.SnapshotsCount = static_cast<uint16_t>(accountState.Balances.snapshots().size());
				std::memcpy(pData, &historicalHeader, sizeof(HistoricalSnapshotsHeader));
				pData += sizeof(HistoricalSnapshotsHeader);

				pUint64Data = reinterpret_cast<uint64_t*>(pData);
				for (const auto& snapshot : accountState.Balances.snapshots()) {
					*pUint64Data++ = snapshot.Amount.unwrap();
					*pUint64Data++ = snapshot.BalanceHeight.unwrap();
				}

				return buffer;
			}
		};
	}

	// endregion

	// region traits

	namespace {
		struct FullTraits {
			using Serializer = AccountStateSerializer;

			static constexpr auto Has_Historical_Snapshots = true;

			static size_t bufferPaddingSize(const AccountState&) {
				return 0;
			}

			static void AssertEqual(const AccountState& expected, const AccountState& actual) {
				test::AssertEqual(expected, actual);
			}
		};

		// notice that CopyToBuffer always writes historical importances, so tests implicitly verify
		// that ex-history serialized data is a subset of full serialized data
		struct NonHistoricalTraits {
			using Serializer = AccountStateNonHistoricalSerializer;

			static constexpr auto Has_Historical_Snapshots = false;

			static size_t bufferPaddingSize(const AccountState& accountState) {
				return accountState.Balances.snapshots().size() * sizeof(model::BalanceSnapshot) + sizeof(HistoricalSnapshotsHeader);
			}

			static void AssertEqual(const AccountState& expected, const AccountState& actual) {
				// strip historical importances
				auto expectedCopy = expected;
				expectedCopy.Balances.cleanUpSnaphots();
				test::AssertEqual(expectedCopy, actual);
			}
		};
	}

#define SERIALIZER_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<FullTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_NonHistorical) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<NonHistoricalTraits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	// endregion

	// region Save

	namespace {


		template<typename TTraits,uint32_t TVersion, typename TAction>
		void AssertCanSaveValueWithMosaics(size_t numMosaics, TAction action) {
			// Arrange:
			std::vector<uint8_t> buffer;
			mocks::MockMemoryStream stream(buffer);

			// - create a random account state
			auto originalAccountState = CreateRandomAccountState(numMosaics, TVersion);

			// Act:
			TTraits::Serializer::Save(originalAccountState, stream);
			auto packetSize = VersionedStateSerializerUtils<TTraits, TVersion>::CalculatePackedSize(originalAccountState);
			auto paddingSize = TTraits::bufferPaddingSize(originalAccountState);
			// Assert:
			ASSERT_EQ( packetSize - paddingSize , buffer.size()) << "\n Packed Size: " << packetSize << "Padding Size: " << paddingSize;

			const auto& savedAccountStateHeader = reinterpret_cast<const AccountStateHeader<TVersion>&>(*(buffer.data() + sizeof(VersionType)));

			EXPECT_EQ(numMosaics, savedAccountStateHeader.MosaicsCount);
			action(originalAccountState, savedAccountStateHeader);

			// Sanity: no stream flushes
			EXPECT_EQ(0u, stream.numFlushes());
		}

		template<typename TTraits, uint32_t TVersion>
		void AssertCanSaveValueWithMosaics(size_t numMosaics) {
			// Act:
			AssertCanSaveValueWithMosaics<TTraits, TVersion>(numMosaics, [](const auto& originalAccountState, const auto& savedAccountStateHeader) {
				// Assert:
				auto savedAccountState = VersionedStateSerializerUtils<TTraits, TVersion>::CopyHeaderToAccountState(savedAccountStateHeader);
				TTraits::AssertEqual(originalAccountState, savedAccountState);
			});
		}
		template<typename TTraits, uint32_t TVersion>
		void VerifyMosaicOrder(const AccountState& ignore, const AccountStateHeader<TVersion>& savedAccountStateHeader) {
			auto lastMosaicId = MosaicId();
			const auto* pMosaic = VersionedStateSerializerUtils<TTraits, TVersion>::GetMosaicPointer(savedAccountStateHeader);
			for (auto i = 0u; i < savedAccountStateHeader.MosaicsCount; ++i, ++pMosaic) {
				EXPECT_LT(lastMosaicId, pMosaic->MosaicId) << "expected ordering at " << i;
				lastMosaicId = pMosaic->MosaicId;
			}
		}
	}

	SERIALIZER_TEST(CanSaveValue) {
		// Assert:
		AssertCanSaveValueWithMosaics<TTraits, 1>(3);
		AssertCanSaveValueWithMosaics<TTraits, 2>(3);
	}

	SERIALIZER_TEST(CanSaveValueWithManyMosaics) {
		// Assert:
		AssertCanSaveValueWithMosaics<TTraits, 1>(GetManyMosaicsCount());
		AssertCanSaveValueWithMosaics<TTraits, 2>(GetManyMosaicsCount());
	}

	SERIALIZER_TEST(MosaicsAreSavedInSortedOrder) {
		// Assert:
		AssertCanSaveValueWithMosaics<TTraits, 1>(128, VerifyMosaicOrder<TTraits,1>);
		AssertCanSaveValueWithMosaics<TTraits, 2>(128, VerifyMosaicOrder<TTraits,2>);
	}

	// endregion

	// region Load

	namespace {
		template<typename TTraits>
		static void AssertCannotLoad(io::InputStream& inputStream) {
			// Assert:
			EXPECT_THROW(TTraits::Serializer::Load(inputStream), catapult_runtime_error);
		}

		template<typename TTraits, uint32_t TVersion>
		void AssertCanLoadValueWithMosaics(size_t numMosaics) {
			// Arrange: create a random account info
			auto originalAccountState = CreateRandomAccountState(numMosaics, TVersion);
			auto buffer = VersionedStateSerializerUtils<TTraits, TVersion>::CopyToBuffer(originalAccountState);

			// Act: load the account state
			mocks::MockMemoryStream stream(buffer);
			auto result = TTraits::Serializer::Load(stream);

			// Assert:
			EXPECT_EQ(numMosaics, result.Balances.size());
			TTraits::AssertEqual(originalAccountState, result);
		}
		template<typename TTraits, uint32_t TVersion>
		void AssertCannotLoadAccountInfoExtendingPastEndOfStream(size_t numMosaics) {
			auto originalAccountState = CreateRandomAccountState(numMosaics, TVersion);
			auto buffer = VersionedStateSerializerUtils<TTraits, TVersion>::CopyToBuffer(originalAccountState);

			// - size the buffer one byte too small
			buffer.resize(buffer.size() - TTraits::bufferPaddingSize(originalAccountState) - 1);
			mocks::MockMemoryStream stream(buffer);
			// Act + Assert:
			AssertCannotLoad<TTraits>(stream);
		}

	}

	SERIALIZER_TEST(CanLoadValueWithNoMosaics) {
		// Assert:
		AssertCanLoadValueWithMosaics<TTraits, 1>(0);
		AssertCanLoadValueWithMosaics<TTraits, 2>(0);
	}

	SERIALIZER_TEST(CanLoadValueWithSomeMosaics) {
		// Assert:
		AssertCanLoadValueWithMosaics<TTraits, 1>(3);
		AssertCanLoadValueWithMosaics<TTraits, 2>(3);
	}

	SERIALIZER_TEST(CanLoadValueWithManyMosaics) {
		// Assert:
		AssertCanLoadValueWithMosaics<TTraits, 1>(GetManyMosaicsCount());
		AssertCanLoadValueWithMosaics<TTraits, 2>(GetManyMosaicsCount());
	}

	SERIALIZER_TEST(CannotLoadAccountInfoExtendingPastEndOfStream) {
		AssertCannotLoadAccountInfoExtendingPastEndOfStream<TTraits, 1>(2);
		AssertCannotLoadAccountInfoExtendingPastEndOfStream<TTraits, 2>(2);
	}

	// endregion
}}