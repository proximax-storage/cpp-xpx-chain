/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/state/SdaOfferGroupEntry.h"
#include "tests/test/SdaExchangeTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS SdaOfferGroupEntryTests

    namespace {
        struct SdaOfferGroupTraits {
            static SdaOfferGroupMap& GetSdaOfferGroup(SdaOfferGroupEntry& entry) {
                return entry.sdaOfferGroup();
            }

            static std::vector<SdaOfferBasicInfo> GenerateSdaOfferBasicInfo(uint8_t offerCount = 5) {
                return test::GenerateSdaOfferBasicInfo(offerCount);
            }

            static SdaOfferBasicInfo CreateSdaOfferBasicInfo(const SdaOfferBasicInfo& offer) {
                return SdaOfferBasicInfo{ offer };
            }
        };
    }

#define TRAITS_BASED_TEST(TEST_NAME) \
    template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
    TEST(TEST_CLASS, TEST_NAME##_SdaOfferGroup) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<SdaOfferGroupTraits>(); } \
    template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

    TRAITS_BASED_TEST(CanArrangeSdaOffersFromSmallToBig) {
        // Act:
		Hash256 groupHash = test::GenerateRandomByteArray<Hash256>();
		Key owner[3];
		for (int o = 0; o < owner->size(); o++)
			owner[o] = test::GenerateRandomByteArray<Key>();

		auto entry = SdaOfferGroupEntry(groupHash);
        std::vector<SdaOfferBasicInfo> info;
        info.emplace_back(SdaOfferBasicInfo{ owner[1], Amount(10), Height(1) });
        info.emplace_back(SdaOfferBasicInfo{ owner[2], Amount(300), Height(1) });
        info.emplace_back(SdaOfferBasicInfo{ owner[3], Amount(150), Height(1) });
        entry.sdaOfferGroup().emplace(groupHash, info);

        auto expectedEntry = SdaOfferGroupEntry(groupHash);
		std::vector<SdaOfferBasicInfo> expectedInfo;
		expectedInfo.emplace_back(SdaOfferBasicInfo{ owner[1], Amount(10), Height(1) });
		expectedInfo.emplace_back(SdaOfferBasicInfo{ owner[3], Amount(150), Height(1) });
		expectedInfo.emplace_back(SdaOfferBasicInfo{ owner[2], Amount(300), Height(1) });
        expectedEntry.sdaOfferGroup().emplace(groupHash, expectedInfo);

        // Act:
        entry.smallToBig(entry.groupHash(), entry.sdaOfferGroup());

        // Assert:
        test::AssertEqualSdaOfferGroupData(expectedEntry, entry);
    }

    TRAITS_BASED_TEST(CanArrangeSdaOffersFromSmallToBigSortedByEarliestExpiry) {
        // Act:
        Hash256 groupHash = test::GenerateRandomByteArray<Hash256>();
		Key owner[5];
		for (int o = 0; o < owner->size(); o++)
			owner[o] = test::GenerateRandomByteArray<Key>();

        auto entry = SdaOfferGroupEntry(groupHash);
        std::vector<SdaOfferBasicInfo> info;
        info.emplace_back(SdaOfferBasicInfo{ owner[1], Amount(10), Height(1) });
        info.emplace_back(SdaOfferBasicInfo{ owner[2], Amount(300), Height(1) });
        info.emplace_back(SdaOfferBasicInfo{ owner[3], Amount(150), Height(3) });
        info.emplace_back(SdaOfferBasicInfo{ owner[4], Amount(50), Height(3) });
        info.emplace_back(SdaOfferBasicInfo{ owner[5], Amount(50), Height(2) });
        entry.sdaOfferGroup().emplace(groupHash, info);

        auto expectedEntry = SdaOfferGroupEntry(groupHash);
        std::vector<SdaOfferBasicInfo> expectedInfo;
        expectedInfo.emplace_back(SdaOfferBasicInfo{ owner[1], Amount(10), Height(1) });
        expectedInfo.emplace_back(SdaOfferBasicInfo{ owner[2], Amount(300), Height(1) });
        expectedInfo.emplace_back(SdaOfferBasicInfo{ owner[5], Amount(50), Height(2) });
        expectedInfo.emplace_back(SdaOfferBasicInfo{ owner[4], Amount(50), Height(3) });
        expectedInfo.emplace_back(SdaOfferBasicInfo{ owner[3], Amount(150), Height(3) });
        expectedEntry.sdaOfferGroup().emplace(groupHash, expectedInfo);

        // Act:
        entry.smallToBigSortedByEarliestExpiry(entry.groupHash(), entry.sdaOfferGroup());

        // Assert:
        test::AssertEqualSdaOfferGroupData(expectedEntry, entry);
    }

    TRAITS_BASED_TEST(CanArrangeSdaOffersFromBigToSmall) {
        // Act:
        Hash256 groupHash = test::GenerateRandomByteArray<Hash256>();
		Key owner[3];
		for (int o = 0; o < owner->size(); o++)
			owner[o] = test::GenerateRandomByteArray<Key>();

        auto entry = SdaOfferGroupEntry(groupHash);
        std::vector<SdaOfferBasicInfo> info;
        info.emplace_back(SdaOfferBasicInfo{ owner[1], Amount(10), Height(1) });
        info.emplace_back(SdaOfferBasicInfo{ owner[2], Amount(300), Height(1) });
        info.emplace_back(SdaOfferBasicInfo{ owner[3], Amount(150), Height(1) });
        entry.sdaOfferGroup().emplace(groupHash, info);

        auto expectedEntry = SdaOfferGroupEntry(groupHash);
        std::vector<SdaOfferBasicInfo> expectedInfo;
        expectedInfo.emplace_back(SdaOfferBasicInfo{ owner[2], Amount(300), Height(1) });
        expectedInfo.emplace_back(SdaOfferBasicInfo{ owner[3], Amount(150), Height(1) });
        expectedInfo.emplace_back(SdaOfferBasicInfo{ owner[1], Amount(10), Height(1) });
        expectedEntry.sdaOfferGroup().emplace(groupHash, expectedInfo);       

        // Act:
        entry.bigToSmall(entry.groupHash(), entry.sdaOfferGroup());

        // Assert:
        test::AssertEqualSdaOfferGroupData(expectedEntry, entry);
    }

    TRAITS_BASED_TEST(CanArrangeSdaOffersFromBigToSmallSortedByEarliestExpiry) {
        // Act:
        Hash256 groupHash = test::GenerateRandomByteArray<Hash256>();
		Key owner[5];
		for (int o = 0; o < owner->size(); o++)
			owner[o] = test::GenerateRandomByteArray<Key>();

        auto entry = SdaOfferGroupEntry(groupHash);
        std::vector<SdaOfferBasicInfo> info;
        info.emplace_back(SdaOfferBasicInfo{ owner[1], Amount(10), Height(1) });
        info.emplace_back(SdaOfferBasicInfo{ owner[2], Amount(300), Height(1) });
        info.emplace_back(SdaOfferBasicInfo{ owner[3], Amount(150), Height(3) });
        info.emplace_back(SdaOfferBasicInfo{ owner[4], Amount(50), Height(3) });
        info.emplace_back(SdaOfferBasicInfo{ owner[5], Amount(50), Height(2) });
        entry.sdaOfferGroup().emplace(groupHash, info);

        auto expectedEntry = SdaOfferGroupEntry(groupHash);
        std::vector<SdaOfferBasicInfo> expectedInfo;       
        expectedInfo.emplace_back(SdaOfferBasicInfo{ owner[2], Amount(300), Height(1) });
        expectedInfo.emplace_back(SdaOfferBasicInfo{ owner[1], Amount(10), Height(1) });
        expectedInfo.emplace_back(SdaOfferBasicInfo{ owner[5], Amount(50), Height(2) });
        expectedInfo.emplace_back(SdaOfferBasicInfo{ owner[3], Amount(150), Height(3) });
        expectedInfo.emplace_back(SdaOfferBasicInfo{ owner[4], Amount(50), Height(3) });
        expectedEntry.sdaOfferGroup().emplace(groupHash, expectedInfo);

        // Act:
        entry.bigToSmallSortedByEarliestExpiry(entry.groupHash(), entry.sdaOfferGroup());

        // Assert:
        test::AssertEqualSdaOfferGroupData(expectedEntry, entry);
    }

    TRAITS_BASED_TEST(CanArrangeSdaOffersFromExactOrClosest) {
        // Act:
        Hash256 groupHash = test::GenerateRandomByteArray<Hash256>();
		Key owner[5];
		for (int o = 0; o < owner->size(); o++)
			owner[o] = test::GenerateRandomByteArray<Key>();

        auto entry = SdaOfferGroupEntry(groupHash);
        std::vector<SdaOfferBasicInfo> info;
        info.emplace_back(SdaOfferBasicInfo{ owner[1], Amount(10), Height(1) });
        info.emplace_back(SdaOfferBasicInfo{ owner[2], Amount(300), Height(1) });
        info.emplace_back(SdaOfferBasicInfo{ owner[3], Amount(150), Height(3) });
        info.emplace_back(SdaOfferBasicInfo{ owner[4], Amount(50), Height(3) });
        info.emplace_back(SdaOfferBasicInfo{ owner[5], Amount(50), Height(2) });
        entry.sdaOfferGroup().emplace(groupHash, info);

        auto expectedEntry = SdaOfferGroupEntry(groupHash);
        std::vector<SdaOfferBasicInfo> expectedInfo;
        expectedInfo.emplace_back(SdaOfferBasicInfo{ owner[4], Amount(50), Height(3) });
        expectedInfo.emplace_back(SdaOfferBasicInfo{ owner[5], Amount(50), Height(2) });
        expectedInfo.emplace_back(SdaOfferBasicInfo{ owner[1], Amount(10), Height(1) });
        expectedInfo.emplace_back(SdaOfferBasicInfo{ owner[3], Amount(150), Height(3) });
        expectedInfo.emplace_back(SdaOfferBasicInfo{ owner[2], Amount(300), Height(1) });
        expectedEntry.sdaOfferGroup().emplace(groupHash, expectedInfo);

        // Act:
        entry.exactOrClosest(entry.groupHash(), Amount(40), entry.sdaOfferGroup());

        // Assert:
        test::AssertEqualSdaOfferGroupData(expectedEntry, entry);
    }

    TEST(TEST_CLASS, CanCreateSdaOfferGroupEntry) {
        // Act:
        auto groupHash = test::GenerateRandomByteArray<Hash256>();
        auto entry = SdaOfferGroupEntry(groupHash);

        // Assert:
        EXPECT_EQ(groupHash, entry.groupHash());
    }

    TRAITS_BASED_TEST(CanAccessSdaOfferGroup) {
        // Arrange:
        auto groupHash = test::GenerateRandomByteArray<Hash256>();
        auto entry = SdaOfferGroupEntry(groupHash);
        auto& offers = TTraits::GetSdaOfferGroup(entry);
        ASSERT_TRUE(offers.empty());
        auto offer = TTraits::GenerateSdaOfferBasicInfo(5);

        // Act:
        offers.emplace(groupHash, offer);

        // Assert:
        offers = TTraits::GetSdaOfferGroup(entry);
        ASSERT_EQ(1, offers.size());
        test::AssertSdaOfferBasicInfo(offer, offers.at(groupHash));
    }

    TEST(TEST_CLASS, AddSdaOfferToGroup) {
        // Arrange:
        auto groupHash = test::GenerateRandomByteArray<Hash256>();
        auto entry = SdaOfferGroupEntry(groupHash);
        model::SdaOfferWithOwnerAndDuration sdaOffer1{ {{ UnresolvedMosaicId(1), Amount(10) }, { UnresolvedMosaicId(2), Amount(100) }}, Key(), BlockDuration(100)};
        model::SdaOfferWithOwnerAndDuration sdaOffer2{ {{ UnresolvedMosaicId(1), Amount(100) }, { UnresolvedMosaicId(2), Amount(10) }}, Key(), BlockDuration(100)};

        auto expectedEntry = SdaOfferGroupEntry(groupHash);
        std::vector<SdaOfferBasicInfo> expectedInfo;
        expectedInfo.emplace_back(SdaOfferBasicInfo{ Key(), Amount(10), Height(1) });
        expectedInfo.emplace_back(SdaOfferBasicInfo{ Key(), Amount(100), Height(2) });
        expectedEntry.sdaOfferGroup().emplace(groupHash, expectedInfo);

        // Act:
        entry.addSdaOfferToGroup(groupHash, &sdaOffer1, Height(1));
        entry.addSdaOfferToGroup(groupHash, &sdaOffer2, Height(2));

        // Assert:
        test::AssertEqualSdaOfferGroupData(expectedEntry, entry);
    }

    TEST(TEST_CLASS, RemoveSdaOfferFromGroup) {
        // Arrange:
        auto groupHash = test::GenerateRandomByteArray<Hash256>();
        auto owner1 = test::GenerateRandomByteArray<Key>();
        auto owner2 = test::GenerateRandomByteArray<Key>();

        auto entry = SdaOfferGroupEntry(groupHash);
        std::vector<state::SdaOfferBasicInfo> info;
        info.emplace_back(SdaOfferBasicInfo{ owner1, Amount(10), Height(1) });
        info.emplace_back(SdaOfferBasicInfo{ owner2, Amount(100), Height(2) });
        info.emplace_back(SdaOfferBasicInfo{ owner1, Amount(500), Height(3) });
        entry.sdaOfferGroup().emplace(groupHash, info);

        auto expectedEntry = SdaOfferGroupEntry(groupHash);
        std::vector<SdaOfferBasicInfo> expectedInfo;
        expectedInfo.emplace_back(SdaOfferBasicInfo{ owner2, Amount(100), Height(1) });
        expectedEntry.sdaOfferGroup().emplace(groupHash, expectedInfo);

        // Act:
        entry.removeSdaOfferFromGroup(groupHash, owner1);
        // Assert:
        test::AssertEqualSdaOfferGroupData(expectedEntry, entry);
    }
}}
