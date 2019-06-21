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

		struct AccountStateHeader {
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

		struct MAY_ALIAS HistoricalSnapshotsHeader {
			uint16_t SnapshotsCount;
		};

#pragma pack(pop)

		size_t CalculatePackedSize(const AccountState& accountState) {
			return sizeof(AccountStateHeader) + sizeof(HistoricalSnapshotsHeader)
					+ accountState.Balances.size() * sizeof(model::Mosaic)
					+ accountState.Balances.snapshots().size() * sizeof(model::BalanceSnapshot);
		}

		const model::Mosaic* GetMosaicPointer(const AccountStateHeader& header) {
			const auto* pHeaderData = reinterpret_cast<const uint8_t*>(&header + 1);
			return reinterpret_cast<const model::Mosaic*>(pHeaderData);
		}

		template<typename TTraits>
		AccountState CopyHeaderToAccountState(const AccountStateHeader& header) {
			auto accountState = AccountState(header.Address, header.AddressHeight);
			accountState.PublicKey = header.PublicKey;
			accountState.PublicKeyHeight = header.PublicKeyHeight;

			accountState.AccountType = header.AccountType;
			accountState.LinkedAccountKey = header.LinkedAccountKey;

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

		std::vector<uint8_t> CopyToBuffer(const AccountState& accountState) {
			std::vector<uint8_t> buffer(CalculatePackedSize(accountState));

			AccountStateHeader header;
			header.Address = accountState.Address;
			header.AddressHeight = accountState.AddressHeight;
			header.PublicKey = accountState.PublicKey;
			header.PublicKeyHeight = accountState.PublicKeyHeight;

			header.AccountType = accountState.AccountType;
			header.LinkedAccountKey = accountState.LinkedAccountKey;

			header.OptimizedMosaicId = accountState.Balances.optimizedMosaicId();
			header.TrackedMosaicId = accountState.Balances.trackedMosaicId();
			header.MosaicsCount = static_cast<uint16_t>(accountState.Balances.size());

			auto* pData = buffer.data();
			std::memcpy(pData, &header, sizeof(AccountStateHeader));
			pData += sizeof(AccountStateHeader);

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
		AccountState CreateRandomAccountState(size_t numMosaics) {
			auto accountState = AccountState(test::GenerateRandomAddress(), Height(123));
			test::FillWithRandomData(accountState.PublicKey);
			accountState.PublicKeyHeight = Height(234);

			accountState.AccountType = static_cast<AccountType>(33);
			test::FillWithRandomData(accountState.LinkedAccountKey);

			test::RandomFillAccountData(0, accountState, numMosaics, numMosaics);
			accountState.Balances.optimize(test::GenerateRandomValue<MosaicId>());
			return accountState;
		}

		template<typename TTraits, typename TAction>
		void AssertCanSaveValueWithMosaics(size_t numMosaics, TAction action) {
			// Arrange:
			std::vector<uint8_t> buffer;
			mocks::MockMemoryStream stream(buffer);

			// - create a random account state
			auto originalAccountState = CreateRandomAccountState(numMosaics);

			// Act:
			TTraits::Serializer::Save(originalAccountState, stream);

			// Assert:
			ASSERT_EQ(CalculatePackedSize(originalAccountState) - TTraits::bufferPaddingSize(originalAccountState), buffer.size());

			const auto& savedAccountStateHeader = reinterpret_cast<const AccountStateHeader&>(*buffer.data());
			EXPECT_EQ(numMosaics, savedAccountStateHeader.MosaicsCount);
			action(originalAccountState, savedAccountStateHeader);

			// Sanity: no stream flushes
			EXPECT_EQ(0u, stream.numFlushes());
		}

		template<typename TTraits>
		void AssertCanSaveValueWithMosaics(size_t numMosaics) {
			// Act:
			AssertCanSaveValueWithMosaics<TTraits>(numMosaics, [](const auto& originalAccountState, const auto& savedAccountStateHeader) {
				// Assert:
				auto savedAccountState = CopyHeaderToAccountState<TTraits>(savedAccountStateHeader);
				TTraits::AssertEqual(originalAccountState, savedAccountState);
			});
		}
	}

	SERIALIZER_TEST(CanSaveValue) {
		// Assert:
		AssertCanSaveValueWithMosaics<TTraits>(3);
	}

	SERIALIZER_TEST(CanSaveValueWithManyMosaics) {
		// Assert:
		AssertCanSaveValueWithMosaics<TTraits>(GetManyMosaicsCount());
	}

	SERIALIZER_TEST(MosaicsAreSavedInSortedOrder) {
		// Assert:
		AssertCanSaveValueWithMosaics<TTraits>(128, [](const auto&, const auto& savedAccountStateHeader) {
			auto lastMosaicId = MosaicId();
			const auto* pMosaic = GetMosaicPointer(savedAccountStateHeader);
			for (auto i = 0u; i < savedAccountStateHeader.MosaicsCount; ++i, ++pMosaic) {
				EXPECT_LT(lastMosaicId, pMosaic->MosaicId) << "expected ordering at " << i;

				lastMosaicId = pMosaic->MosaicId;
			}
		});
	}

	// endregion

	// region Load

	namespace {
		template<typename TTraits>
		static void AssertCannotLoad(io::InputStream& inputStream) {
			// Assert:
			EXPECT_THROW(TTraits::Serializer::Load(inputStream), catapult_runtime_error);
		}

		template<typename TTraits>
		void AssertCanLoadValueWithMosaics(size_t numMosaics) {
			// Arrange: create a random account info
			auto originalAccountState = CreateRandomAccountState(numMosaics);
			auto buffer = CopyToBuffer(originalAccountState);

			// Act: load the account state
			mocks::MockMemoryStream stream(buffer);
			auto result = TTraits::Serializer::Load(stream);

			// Assert:
			EXPECT_EQ(numMosaics, result.Balances.size());
			TTraits::AssertEqual(originalAccountState, result);
		}
	}

	SERIALIZER_TEST(CanLoadValueWithNoMosaics) {
		// Assert:
		AssertCanLoadValueWithMosaics<TTraits>(0);
	}

	SERIALIZER_TEST(CanLoadValueWithSomeMosaics) {
		// Assert:
		AssertCanLoadValueWithMosaics<TTraits>(3);
	}

	SERIALIZER_TEST(CanLoadValueWithManyMosaics) {
		// Assert:
		AssertCanLoadValueWithMosaics<TTraits>(GetManyMosaicsCount());
	}

	SERIALIZER_TEST(CannotLoadAccountInfoExtendingPastEndOfStream) {
		// Arrange: create a random account info
		auto originalAccountState = CreateRandomAccountState(2);
		auto buffer = CopyToBuffer(originalAccountState);

		// - size the buffer one byte too small
		buffer.resize(buffer.size() - TTraits::bufferPaddingSize(originalAccountState) - 1);
		mocks::MockMemoryStream stream(buffer);

		// Act + Assert:
		AssertCannotLoad<TTraits>(stream);
	}

	// endregion
}}
