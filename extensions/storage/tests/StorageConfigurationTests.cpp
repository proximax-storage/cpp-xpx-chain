/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/StorageConfiguration.h"
#include "tests/test/nodeps/ConfigurationTestUtils.h"

namespace catapult { namespace config {

    namespace {
        struct StorageConfigurationTraits {
            using ConfigurationType = storage::StorageConfiguration;

            static utils::ConfigurationBag::ValuesContainer CreateProperties() {
                return {
                        {
                                "replicator",
                                {
                                        { "port", "5000" },
                                        { "transactionTimeout", "1s" },
                                        { "storageDirectory", "/tmp/storage" },
                                        { "sandboxDirectory", "/tmp/storage/sandbox" },
                                        { "useTcpSocket", "true" },
                                }
                        }
                };
            }

            static bool IsSectionOptional(const std::string& name) {
                return false;
            }

            static bool IsPropertyOptional(const std::string&) {
                return false;
            }

            static bool SupportsUnknownProperties() {
                return false;
            }

            static void AssertZero(const storage::StorageConfiguration& config) {
                // Assert:
                EXPECT_EQ("", config.Port);
                EXPECT_EQ(utils::TimeSpan::FromHours(0), config.TransactionTimeout);
                EXPECT_EQ("", config.StorageDirectory);
                EXPECT_EQ("", config.SandboxDirectory);
                EXPECT_EQ(false, config.UseTcpSocket);
            }

            static void AssertCustom(const storage::StorageConfiguration& config) {
                // Assert:
                EXPECT_EQ("5000", config.Port);
                EXPECT_EQ(utils::TimeSpan::FromSeconds(1), config.TransactionTimeout);
                EXPECT_EQ("/tmp/storage", config.StorageDirectory);
                EXPECT_EQ("/tmp/storage/sandbox", config.SandboxDirectory);
                EXPECT_EQ(true, config.UseTcpSocket);
            }
        };
    }

#define TEST_CLASS StorageConfigurationTests

    TEST(TEST_CLASS, CanCreateUninitializedStorageConfiguration) {
		test::AssertCanCreateUninitializedConfiguration<StorageConfigurationTraits>();
	}

	TEST(TEST_CLASS, CannotLoadStorageConfigurationFromEmptyBag) {
		test::AssertCannotLoadConfigurationFromEmptyBag<StorageConfigurationTraits>();
	}

    TEST(TEST_CLASS, LoadStorageConfigurationFromBagWithMissingProperty) {
		test::AssertLoadConfigurationFromBagWithMissingProperty<StorageConfigurationTraits>();
	}

	TEST(TEST_CLASS, LoadStorageConfigurationFromBagWithUnknownProperty) {
		test::AssertLoadConfigurationFromBagWithUnknownProperty<StorageConfigurationTraits>();
	}

	TEST(TEST_CLASS, LoadStorageConfigurationFromBagWithUnknownSection) {
		test::AssertLoadConfigurationFromBagWithUnknownSection<StorageConfigurationTraits>();
	}

	TEST(TEST_CLASS, CanLoadCustomStorageConfigurationFromBag) {
		test::AssertCanLoadCustomConfigurationFromBag<StorageConfigurationTraits>();
	}
}}
