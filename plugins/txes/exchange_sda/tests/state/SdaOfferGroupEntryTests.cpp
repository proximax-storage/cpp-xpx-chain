/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/state/SdaOfferGroupEntry.h"
#include "tests/test/SdaExchangeTestUtils.h"
#include "tests/TestHarness.h"
#include <array>

namespace catapult { namespace state {

#define TEST_CLASS SdaOfferGroupEntryTests

    namespace {
        struct SdaOfferGroupTraits {
            static SdaOfferGroupVector& GetSdaOfferGroup(SdaOfferGroupEntry& entry) {
                return entry.sdaOfferGroup();
            }

            static SdaOfferGroupVector GenerateSdaOfferGroupVector(uint16_t offerCount = 5) {
                SdaOfferGroupVector offerGroup;
                for (auto i = 0u; i < offerCount; ++i) {
                    offerGroup.push_back(test::GenerateSdaOfferBasicInfo());
                }
                return offerGroup;
            }
        };
    }

#define TRAITS_BASED_TEST(TEST_NAME) \
    template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
    TEST(TEST_CLASS, TEST_NAME) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<SdaOfferGroupTraits>(); } \
    template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

    TRAITS_BASED_TEST(CanArrangeSdaOffersFromSmallToBig) {
        // Act:
        Hash256 groupHash = test::GenerateRandomByteArray<Hash256>();
        std::array<Key, 3> owner;
        for (int o = 0; o < owner.size(); o++)
            owner[o] = test::GenerateRandomByteArray<Key>();

        auto entry = SdaOfferGroupEntry(groupHash);
        entry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[0], Amount(10), Height(1) });
        entry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[1], Amount(300), Height(1) });
        entry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[2], Amount(150), Height(1) });

        auto expectedEntry = SdaOfferGroupEntry(groupHash);
        expectedEntry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[0], Amount(10), Height(1) });
        expectedEntry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[2], Amount(150), Height(1) });
        expectedEntry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[1], Amount(300), Height(1) });;

        // Act:
        entry.smallToBig(entry.sdaOfferGroup());

        // Assert:
        test::AssertEqualSdaOfferGroupData(expectedEntry, entry);
    }

    TRAITS_BASED_TEST(CanArrangeSdaOffersFromSmallToBigSortedByEarliestExpiry) {
        // Act:
        Hash256 groupHash = test::GenerateRandomByteArray<Hash256>();
        std::array<Key, 5> owner;
        for (int o = 0; o < owner.size(); o++)
            owner[o] = test::GenerateRandomByteArray<Key>();

        auto entry = SdaOfferGroupEntry(groupHash);
        entry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[0], Amount(10), Height(1) });
        entry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[1], Amount(300), Height(1) });
        entry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[2], Amount(150), Height(3) });
        entry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[3], Amount(50), Height(3) });
        entry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[4], Amount(50), Height(2) });

        auto expectedEntry = SdaOfferGroupEntry(groupHash);
        expectedEntry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[0], Amount(10), Height(1) });
        expectedEntry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[1], Amount(300), Height(1) });
        expectedEntry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[4], Amount(50), Height(2) });
        expectedEntry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[3], Amount(50), Height(3) });
        expectedEntry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[2], Amount(150), Height(3) });

        // Act:
        entry.smallToBigSortedByEarliestExpiry(entry.sdaOfferGroup());

        // Assert:
        test::AssertEqualSdaOfferGroupData(expectedEntry, entry);
    }

    TRAITS_BASED_TEST(CanArrangeSdaOffersFromBigToSmall) {
        // Act:
        Hash256 groupHash = test::GenerateRandomByteArray<Hash256>();
        std::array<Key, 3> owner;
        for (int o = 0; o < owner.size(); o++)
            owner[o] = test::GenerateRandomByteArray<Key>();

        auto entry = SdaOfferGroupEntry(groupHash);
        entry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[0], Amount(10), Height(1) });
        entry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[1], Amount(300), Height(1) });
        entry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[2], Amount(150), Height(1) });

        auto expectedEntry = SdaOfferGroupEntry(groupHash);
        expectedEntry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[1], Amount(300), Height(1) });
        expectedEntry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[2], Amount(150), Height(1) });
        expectedEntry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[0], Amount(10), Height(1) });      

        // Act:
        entry.bigToSmall(entry.sdaOfferGroup());

        // Assert:
        test::AssertEqualSdaOfferGroupData(expectedEntry, entry);
    }

    TRAITS_BASED_TEST(CanArrangeSdaOffersFromBigToSmallSortedByEarliestExpiry) {
        // Act:
        Hash256 groupHash = test::GenerateRandomByteArray<Hash256>();
        std::array<Key, 5> owner;
        for (int o = 0; o < owner.size(); o++)
            owner[o] = test::GenerateRandomByteArray<Key>();

        auto entry = SdaOfferGroupEntry(groupHash);
        entry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[0], Amount(10), Height(1) });
        entry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[1], Amount(300), Height(1) });
        entry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[2], Amount(150), Height(3) });
        entry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[3], Amount(50), Height(3) });
        entry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[4], Amount(50), Height(2) });

        auto expectedEntry = SdaOfferGroupEntry(groupHash);   
        expectedEntry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[1], Amount(300), Height(1) });
        expectedEntry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[0], Amount(10), Height(1) });
        expectedEntry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[4], Amount(50), Height(2) });
        expectedEntry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[2], Amount(150), Height(3) });
        expectedEntry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[3], Amount(50), Height(3) });

        // Act:
        entry.bigToSmallSortedByEarliestExpiry(entry.sdaOfferGroup());

        // Assert:
        test::AssertEqualSdaOfferGroupData(expectedEntry, entry);
    }

    TRAITS_BASED_TEST(CanArrangeSdaOffersFromExactOrClosest) {
        // Act:
        Hash256 groupHash = test::GenerateRandomByteArray<Hash256>();
        std::array<Key, 5> owner;
        for (int o = 0; o < owner.size(); o++)
            owner[o] = test::GenerateRandomByteArray<Key>();

        auto entry = SdaOfferGroupEntry(groupHash);
        entry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[0], Amount(10), Height(1) });
        entry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[1], Amount(300), Height(1) });
        entry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[2], Amount(150), Height(3) });
        entry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[3], Amount(50), Height(3) });
        entry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[4], Amount(50), Height(2) });

        auto expectedEntry = SdaOfferGroupEntry(groupHash);
        expectedEntry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[3], Amount(50), Height(3) });
        expectedEntry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[4], Amount(50), Height(2) });
        expectedEntry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[0], Amount(10), Height(1) });
        expectedEntry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[2], Amount(150), Height(3) });
        expectedEntry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner[1], Amount(300), Height(1) });

        // Act:
        entry.exactOrClosest(Amount(40), entry.sdaOfferGroup());

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
        auto offer = TTraits::GenerateSdaOfferGroupVector(5);

        // Act:
        for (auto o : offer) {
            offers.emplace_back(o);
        }

        // Assert:
        offers = TTraits::GetSdaOfferGroup(entry);
        ASSERT_EQ(5, offers.size());
        test::AssertSdaOfferGroupInfo(offer, offers);
    }

    TEST(TEST_CLASS, AddSdaOfferToGroup) {
        // Arrange:
        auto groupHash = test::GenerateRandomByteArray<Hash256>();
        auto entry = SdaOfferGroupEntry(groupHash);
        model::SdaOfferWithOwnerAndDuration sdaOffer1{ {{ UnresolvedMosaicId(1), Amount(10) }, { UnresolvedMosaicId(2), Amount(100) }}, Key(), BlockDuration(100)};
        model::SdaOfferWithOwnerAndDuration sdaOffer2{ {{ UnresolvedMosaicId(1), Amount(100) }, { UnresolvedMosaicId(2), Amount(10) }}, Key(), BlockDuration(100)};

        auto expectedEntry = SdaOfferGroupEntry(groupHash);
        expectedEntry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ Key(), Amount(10), Height(1) });
        expectedEntry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ Key(), Amount(100), Height(2) });

        // Act:
        entry.addSdaOfferToGroup(&sdaOffer1, Height(1));
        entry.addSdaOfferToGroup(&sdaOffer2, Height(2));

        // Assert:
        test::AssertEqualSdaOfferGroupData(expectedEntry, entry);
    }

    TEST(TEST_CLASS, RemoveSdaOfferFromGroup) {
        // Arrange:
        auto groupHash = test::GenerateRandomByteArray<Hash256>();
        auto owner1 = test::GenerateRandomByteArray<Key>();
        auto owner2 = test::GenerateRandomByteArray<Key>();

        auto entry = SdaOfferGroupEntry(groupHash);
        entry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner1, Amount(10), Height(1) });
        entry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner2, Amount(100), Height(2) });
        entry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner1, Amount(500), Height(3) });

        auto expectedEntry = SdaOfferGroupEntry(groupHash);
        expectedEntry.sdaOfferGroup().emplace_back(SdaOfferBasicInfo{ owner2, Amount(100), Height(2) });

        // Act:
        entry.removeSdaOfferFromGroup(owner1);

        // Assert:
        test::AssertEqualSdaOfferGroupData(expectedEntry, entry);
    }
}}
