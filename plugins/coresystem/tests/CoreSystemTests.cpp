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

#include "src/CoreSystem.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/plugins/PluginTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace plugins {

	namespace {
		struct CoreSystemTraits {
		public:
			template<typename TAction>
			static void RunTestAfterRegistration(TAction action) {
				// Arrange:
				auto pConfigHolder = config::CreateMockConfigurationHolder();
				PluginManager manager(pConfigHolder, StorageConfiguration());
				RegisterCoreSystem(manager);

				// Act:
				action(manager);
			}

		public:
			static std::vector<model::EntityType> GetTransactionTypes() {
				return {};
			}

			static std::vector<std::string> GetCacheNames() {
				return { "AccountStateCache", "BlockDifficultyCache" };
			}

			static std::vector<ionet::PacketType> GetNonDiagnosticPacketTypes() {
				return { ionet::PacketType::Account_State_Path };
			}

			static std::vector<ionet::PacketType> GetDiagnosticPacketTypes() {
				return { ionet::PacketType::Account_Infos };
			}

			static std::vector<std::string> GetDiagnosticCounterNames() {
				return { "ACNTST C", "ACNTST C HVA", "BLKDIF C" };
			}

			static std::vector<std::string> GetStatelessValidatorNames() {
				return { "NetworkValidator", "TransactionFeeValidator" };
			}

			static std::vector<std::string> GetStatefulValidatorNames() {
				return {
					"EntityVersionValidator",
					"DeadlineValidator",
					"AddressValidator",
					"BalanceTransferValidator",
					"MaxTransactionsValidator",
					"EligibleHarvesterValidator",
					"BalanceDebitValidator",
				};
			}

			static std::vector<std::string> GetObserverNames() {
				return {
					"AccountPublicKeyObserver",
					"SourceChangeObserver",
					"AccountAddressObserver",
					"BalanceTransferObserver",
					"HarvestFeeObserver",
					"TotalTransactionsObserver",
					"SnapshotCleanUpObserver",
					"BlockDifficultyObserver",
					"BlockDifficultyPruningObserver",
					"BalanceDebitObserver",
					"BalanceCreditObserver",
					"LevyTransferObserver",
				};
			}

			static std::vector<std::string> GetPermanentObserverNames() {
				return {
					"AccountPublicKeyObserver",
					"SourceChangeObserver",
					"AccountAddressObserver",
					"BalanceTransferObserver",
					"HarvestFeeObserver",
					"TotalTransactionsObserver",
					"SnapshotCleanUpObserver",
					"BalanceDebitObserver",
					"BalanceCreditObserver",
					"LevyTransferObserver",
				};
			}
		};
	}

	DEFINE_PLUGIN_TESTS(CoreSystemTests, CoreSystemTraits)
}}
