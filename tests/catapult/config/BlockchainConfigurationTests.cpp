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

#include "catapult/config/BlockchainConfiguration.h"
#include "catapult/crypto/KeyPair.h"
#include "catapult/crypto/KeyUtils.h"
#include "catapult/utils/HexParser.h"
#include "tests/test/nodeps/Filesystem.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/TestHarness.h"
#include <boost/filesystem.hpp>

namespace catapult { namespace config {

#define TEST_CLASS BlockchainConfigurationTests

	// region BlockchainConfiguration file io

	namespace {
		const char* Resources_Path = "../resources";
		const char* Config_Filenames[] = {
			"config-extensions-server.properties",
			"config-inflation.properties",
			"config-logging-server.properties",
			"config-immutable.properties",
			"config-network.properties",
			"config-node.properties",
			"config-user.properties",
			"supported-entities.json"
		};

		void AssertDefaultImmutableConfiguration(const config::ImmutableConfiguration& config) {
			// Assert:
			EXPECT_EQ(model::NetworkIdentifier::Mijin_Test, config.NetworkIdentifier);
			EXPECT_EQ(
					utils::ParseByteArray<GenerationHash>("57F7DA205008026C776CB6AED843393F04CD458E0AA2D9F1D5F31A402072B2D6"),
					config.GenerationHash);

			EXPECT_TRUE(config.ShouldEnableVerifiableState);
			EXPECT_TRUE(config.ShouldEnableVerifiableReceipts);

			// - raw values are used instead of test::Default_*_Mosaic_Ids because
			// config files contain mosaic ids when SIGNATURE_SCHEME_NIS1 is disabled
			EXPECT_EQ(MosaicId(0x0DC6'7FBE'1CAD'29E3), config.CurrencyMosaicId);
			EXPECT_EQ(MosaicId(0x0DC6'7FBE'1CAD'29E3), config.HarvestingMosaicId);
			EXPECT_EQ(MosaicId(0x2651'4E2A'1EF3'3824), config.StorageMosaicId);
			EXPECT_EQ(MosaicId(0x6C5D'6875'08AC'9D75), config.StreamingMosaicId);
			EXPECT_EQ(MosaicId(0x77E4'90CC'9B2A'F6F6), config.ReviewMosaicId);
			EXPECT_EQ(MosaicId(0x77E4'90CC'9B2A'F6F6), config.SuperContractMosaicId);
			EXPECT_EQ(MosaicId(0x77E4'90CC'9B2A'F6F6), config.XarMosaicId);

			EXPECT_EQ(Amount(8'999'999'998'000'000), config.InitialCurrencyAtomicUnits);
		}

		void AssertDefaultNetworkConfiguration(const model::NetworkConfiguration& config) {
			// Assert:
			EXPECT_EQ(crypto::ParseKey("B4F12E7C9F6946091E2CB8B6D3A12B50D17CCBBF646386EA27CE2946A7423DCF"), config.Info.PublicKey);

			EXPECT_EQ(utils::TimeSpan::FromSeconds(15), config.BlockGenerationTargetTime);
			EXPECT_EQ(3000u, config.BlockTimeSmoothingFactor);

			EXPECT_EQ(5760u, config.ImportanceGrouping);
			EXPECT_EQ(360u, config.MaxRollbackBlocks);
			EXPECT_EQ(3u, config.MaxDifficultyBlocks);

			EXPECT_EQ(utils::TimeSpan::FromHours(24), config.MaxTransactionLifetime);
			EXPECT_EQ(utils::TimeSpan::FromSeconds(10), config.MaxBlockFutureTime);

			EXPECT_EQ(Amount(9'000'000'000'000'000), config.MaxMosaicAtomicUnits);

			EXPECT_EQ(Importance(8'999'999'998'000'000), config.TotalChainImportance);
			EXPECT_EQ(Amount(100'000'000'000), config.MinHarvesterBalance);
			EXPECT_EQ(10u, config.HarvestBeneficiaryPercentage);

			EXPECT_EQ(360u, config.BlockPruneInterval);
			EXPECT_EQ(200'000u, config.MaxTransactionsPerBlock);

			EXPECT_EQ(true, config.EnableUnconfirmedTransactionMinFeeValidation);

			EXPECT_FALSE(config.Plugins.empty());
		}

		void AssertDefaultNodeConfiguration(const NodeConfiguration& config) {
			// Assert:
			EXPECT_EQ(7900u, config.Port);
			EXPECT_EQ(7901u, config.ApiPort);
			EXPECT_FALSE(config.ShouldAllowAddressReuse);
			EXPECT_FALSE(config.ShouldUseSingleThreadPool);
			EXPECT_TRUE(config.ShouldUseCacheDatabaseStorage);
			EXPECT_TRUE(config.ShouldEnableAutoSyncCleanup);

			EXPECT_TRUE(config.ShouldEnableTransactionSpamThrottling);
			EXPECT_EQ(Amount(10'000'000), config.TransactionSpamThrottlingMaxBoostFee);

			EXPECT_EQ(400u, config.MaxBlocksPerSyncAttempt);
			EXPECT_EQ(utils::FileSize::FromMegabytes(100), config.MaxChainBytesPerSyncAttempt);

			EXPECT_EQ(utils::TimeSpan::FromMinutes(10), config.ShortLivedCacheTransactionDuration);
			EXPECT_EQ(utils::TimeSpan::FromMinutes(100), config.ShortLivedCacheBlockDuration);
			EXPECT_EQ(utils::TimeSpan::FromSeconds(90), config.ShortLivedCachePruneInterval);
			EXPECT_EQ(10'000'000u, config.ShortLivedCacheMaxSize);

			EXPECT_EQ(BlockFeeMultiplier(0), config.MinFeeMultiplier);
			EXPECT_EQ(model::TransactionSelectionStrategy::Oldest, config.TransactionSelectionStrategy);
			EXPECT_EQ(utils::FileSize::FromMegabytes(20), config.UnconfirmedTransactionsCacheMaxResponseSize);
			EXPECT_EQ(1'000'000u, config.UnconfirmedTransactionsCacheMaxSize);

			EXPECT_EQ(utils::TimeSpan::FromSeconds(10), config.ConnectTimeout);
			EXPECT_EQ(utils::TimeSpan::FromSeconds(60), config.SyncTimeout);

			EXPECT_EQ(utils::FileSize::FromKilobytes(512), config.SocketWorkingBufferSize);
			EXPECT_EQ(100u, config.SocketWorkingBufferSensitivity);
			EXPECT_EQ(utils::FileSize::FromMegabytes(150), config.MaxPacketDataSize);

			EXPECT_EQ(4096u, config.BlockDisruptorSize);
			EXPECT_EQ(1u, config.BlockElementTraceInterval);
			EXPECT_EQ(16384u, config.TransactionDisruptorSize);
			EXPECT_EQ(10u, config.TransactionElementTraceInterval);

			EXPECT_TRUE(config.ShouldAbortWhenDispatcherIsFull);
			EXPECT_TRUE(config.ShouldAuditDispatcherInputs);

			EXPECT_EQ(ionet::ConnectionSecurityMode::None, config.OutgoingSecurityMode);
			EXPECT_EQ(ionet::ConnectionSecurityMode::None, config.IncomingSecurityModes);

			EXPECT_EQ(utils::FileSize::FromMegabytes(5), config.MaxCacheDatabaseWriteBatchSize);
			EXPECT_EQ(5'000u, config.MaxTrackedNodes);

			EXPECT_EQ("", config.Local.Host);
			EXPECT_EQ("", config.Local.FriendlyName);
			EXPECT_EQ(0u, config.Local.Version);
			EXPECT_EQ(ionet::NodeRoles::Peer, config.Local.Roles);

			EXPECT_EQ(10u, config.OutgoingConnections.MaxConnections);
			EXPECT_EQ(5u, config.OutgoingConnections.MaxConnectionAge);
			EXPECT_EQ(20u, config.OutgoingConnections.MaxConnectionBanAge);
			EXPECT_EQ(3u, config.OutgoingConnections.NumConsecutiveFailuresBeforeBanning);

			EXPECT_EQ(512u, config.IncomingConnections.MaxConnections);
			EXPECT_EQ(10u, config.IncomingConnections.MaxConnectionAge);
			EXPECT_EQ(20u, config.IncomingConnections.MaxConnectionBanAge);
			EXPECT_EQ(3u, config.IncomingConnections.NumConsecutiveFailuresBeforeBanning);
			EXPECT_EQ(512u, config.IncomingConnections.BacklogSize);
		}

		void AssertDefaultLoggingConfiguration(
				const LoggingConfiguration& config,
				const std::string& expectedLogFilePattern,
				utils::LogLevel expectedFileLogLevel = utils::LogLevel::Info) {
			// Assert:
			// - console (basic)
			EXPECT_EQ(utils::LogSinkType::Sync, config.Console.SinkType);
			EXPECT_EQ(utils::LogLevel::Info, config.Console.Level);
			EXPECT_TRUE(config.Console.ComponentLevels.empty());

			// - console (specific)
			EXPECT_EQ(utils::LogColorMode::Ansi, config.Console.ColorMode);

			// - file (basic)
			EXPECT_EQ(utils::LogSinkType::Async, config.File.SinkType);
			EXPECT_EQ(expectedFileLogLevel, config.File.Level);
			EXPECT_TRUE(config.File.ComponentLevels.empty());

			// - file (specific)
			EXPECT_EQ("logs", config.File.Directory);
			EXPECT_EQ(expectedLogFilePattern, config.File.FilePattern);
			EXPECT_EQ(utils::FileSize::FromMegabytes(25), config.File.RotationSize);
			EXPECT_EQ(utils::FileSize::FromMegabytes(2500), config.File.MaxTotalSize);
			EXPECT_EQ(utils::FileSize::FromMegabytes(100), config.File.MinFreeSpace);
		}

		void AssertDefaultUserConfiguration(const UserConfiguration& config) {
			// Assert:
			EXPECT_EQ("0000000000000000000000000000000000000000000000000000000000000000", config.BootKey);

			EXPECT_EQ("../data", config.DataDirectory);
			EXPECT_EQ(".", config.PluginsDirectory);
		}

		void AssertDefaultExtensionsConfiguration(
				const ExtensionsConfiguration& config,
				const std::vector<std::string>& expectedExtensions) {
			EXPECT_EQ(expectedExtensions, config.Names);
		}

		void AssertDefaultInflationConfiguration(const InflationConfiguration& config) {
			// Assert:
			EXPECT_EQ(2u, config.InflationCalculator.size());
			EXPECT_TRUE(config.InflationCalculator.contains(Height(1), Amount(100)));
			EXPECT_TRUE(config.InflationCalculator.contains(Height(10000), Amount()));
		}

		void AssertDefaultSupportedEntityVersions(const SupportedEntityVersions& config) {
			// Assert:
			EXPECT_EQ(32u, config.size());
		}
	}

	TEST(TEST_CLASS, CannotLoadConfigWhenAnyConfigFileIsMissing) {
		// Arrange:
		for (const auto& filenameToRemove : Config_Filenames) {
			// - copy all files into a temp directory
			test::TempDirectoryGuard tempDir;
			for (const auto& configFilename : Config_Filenames) {
				boost::filesystem::create_directories(tempDir.name());
				boost::filesystem::copy_file(
						boost::filesystem::path(Resources_Path) / configFilename,
						boost::filesystem::path(tempDir.name()) / configFilename);
			}

			// - remove a file
			CATAPULT_LOG(debug) << "removing " << filenameToRemove;
			EXPECT_TRUE(boost::filesystem::remove(boost::filesystem::path(tempDir.name()) / filenameToRemove));

			// Act + Assert: attempt to load the config
			EXPECT_THROW(BlockchainConfiguration::LoadFromPath(tempDir.name(), "server"), catapult_runtime_error);
		}
	}

	TEST(TEST_CLASS, ResourcesDirectoryContainsAllConfigFiles) {
		// Arrange:
		auto resourcesPath = boost::filesystem::path(Resources_Path);
		std::set<boost::filesystem::path> expectedFilenames;
		for (const auto& configFilename : Config_Filenames)
			expectedFilenames.insert(resourcesPath / configFilename);

		// Act: collect filenames
		auto numFiles = 0u;
		std::set<boost::filesystem::path> actualFilenames;
		for (const auto& path : boost::filesystem::directory_iterator(resourcesPath)) {
			CATAPULT_LOG(debug) << "found " << path;
			actualFilenames.insert(path);
			++numFiles;
		}

		// Assert:
		EXPECT_LE(CountOf(Config_Filenames), numFiles);
		for (const auto& expectedFilename : expectedFilenames)
			EXPECT_CONTAINS(actualFilenames, expectedFilename);
	}

	TEST(TEST_CLASS, CanLoadConfigFromResourcesDirectoryWithServerExtensions) {
		// Act: attempt to load from the "real" resources directory
		auto config = BlockchainConfiguration::LoadFromPath(Resources_Path, "server");

		// Assert:
		AssertDefaultImmutableConfiguration(config.Immutable);
		AssertDefaultNetworkConfiguration(config.Network);
		AssertDefaultNodeConfiguration(config.Node);
		AssertDefaultLoggingConfiguration(config.Logging, "catapult_server%4N.log");
		AssertDefaultUserConfiguration(config.User);
		AssertDefaultExtensionsConfiguration(config.Extensions, {
			"extension.eventsource", "extension.harvesting", "extension.syncsource",
			"extension.diagnostics", "extension.hashcache", "extension.networkheight",
			"extension.nodediscovery", "extension.packetserver", "extension.pluginhandlers", "extension.sync",
			"extension.timesync", "extension.transactionsink", "extension.unbondedpruning"
		});
		AssertDefaultInflationConfiguration(config.Inflation);
		AssertDefaultSupportedEntityVersions(config.SupportedEntityVersions);
	}

	TEST(TEST_CLASS, CanLoadConfigFromResourcesDirectoryWithBrokerExtensions) {
		// Act: attempt to load from the "real" resources directory
		auto config = BlockchainConfiguration::LoadFromPath(Resources_Path, "broker");

		// Assert:
		AssertDefaultImmutableConfiguration(config.Immutable);
		AssertDefaultNetworkConfiguration(config.Network);
		AssertDefaultNodeConfiguration(config.Node);
		AssertDefaultLoggingConfiguration(config.Logging, "catapult_broker%4N.log");
		AssertDefaultUserConfiguration(config.User);
		AssertDefaultExtensionsConfiguration(config.Extensions, {
			"extension.addressextraction", "extension.mongo", "extension.zeromq",
			"extension.hashcache"
		});
		AssertDefaultInflationConfiguration(config.Inflation);
		AssertDefaultSupportedEntityVersions(config.SupportedEntityVersions);
	}

	TEST(TEST_CLASS, CanLoadConfigFromResourcesDirectoryWithRecoveryExtensions) {
		// Act: attempt to load from the "real" resources directory
		auto config = BlockchainConfiguration::LoadFromPath(Resources_Path, "recovery");

		// Assert:
		AssertDefaultImmutableConfiguration(config.Immutable);
		AssertDefaultNetworkConfiguration(config.Network);
		AssertDefaultNodeConfiguration(config.Node);
		AssertDefaultLoggingConfiguration(config.Logging, "catapult_recovery%4N.log", utils::LogLevel::Debug);
		AssertDefaultUserConfiguration(config.User);
		AssertDefaultExtensionsConfiguration(config.Extensions, { "extension.hashcache" });
		AssertDefaultInflationConfiguration(config.Inflation);
		AssertDefaultSupportedEntityVersions(config.SupportedEntityVersions);
	}

	// endregion

	// region ToLocalNode

	namespace {
		auto CreateBlockchainConfiguration(const std::string& privateKeyString) {
			test::MutableBlockchainConfiguration config;
			config.Immutable.NetworkIdentifier = model::NetworkIdentifier::Mijin_Test;

			config.Node.Port = 9876;
			config.Node.Local.Host = "alice.com";
			config.Node.Local.FriendlyName = "a GREAT node";
			config.Node.Local.Version = 123;
			config.Node.Local.Roles = ionet::NodeRoles::Api;

			config.User.BootKey = privateKeyString;
			return config.ToConst();
		}
	}

	TEST(TEST_CLASS, CanExtractLocalNodeFromConfiguration) {
		// Arrange:
		auto privateKeyString = test::GenerateRandomHexString(2 * Key_Size);
		auto keyPair = crypto::KeyPair::FromString(privateKeyString);
		auto config = CreateBlockchainConfiguration(privateKeyString);

		// Act:
		auto node = ToLocalNode(config);

		// Assert:
		EXPECT_EQ(keyPair.publicKey(), node.identityKey());

		const auto& endpoint = node.endpoint();
		EXPECT_EQ("alice.com", endpoint.Host);
		EXPECT_EQ(9876u, endpoint.Port);

		const auto& metadata = node.metadata();
		EXPECT_EQ(model::NetworkIdentifier::Mijin_Test, metadata.NetworkIdentifier);
		EXPECT_EQ("a GREAT node", metadata.Name);
		EXPECT_EQ(ionet::NodeVersion(123), metadata.Version);
		EXPECT_EQ(ionet::NodeRoles::Api, metadata.Roles);
	}

	// endregion
}}
