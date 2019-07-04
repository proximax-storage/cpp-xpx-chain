/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/plugins/PluginUtils.h"
#include "src/plugins/MetadataPlugin.h"
#include "plugins/txes/metadata/src/model/MetadataEntityType.h"
#include "tests/test/plugins/PluginTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace plugins {

    namespace {
        struct MetadataPluginTraits {
        public:
            template<typename TAction>
            static void RunTestAfterRegistration(TAction action) {
                // Arrange:
                auto config = model::BlockChainConfiguration::Uninitialized();
                config.Plugins.emplace(PLUGIN_NAME(metadata), utils::ConfigurationBag({
                {
                    "",
                    {
                        { "maxFields", "10" },
                        { "maxFieldKeySize", "128" },
                        { "maxFieldValueSize", "1024" },
                    }
                }}));

				auto pConfigHolder = std::make_shared<config::LocalNodeConfigurationHolder>();
				pConfigHolder->SetBlockChainConfig(Height{0}, config);
				PluginManager manager(pConfigHolder, StorageConfiguration());
                RegisterMetadataSubsystem(manager);

                // Act:
                action(manager);
            }

        public:
            static std::vector<model::EntityType> GetTransactionTypes() {
                return {
                    model::Entity_Type_Address_Metadata,
                    model::Entity_Type_Mosaic_Metadata,
                    model::Entity_Type_Namespace_Metadata
                };
            }

            static std::vector<std::string> GetCacheNames() {
                return { "MetadataCache" };
            }

            static std::vector<ionet::PacketType> GetDiagnosticPacketTypes() {
                return { ionet::PacketType::Metadata_Infos };
            }

            static std::vector<ionet::PacketType> GetNonDiagnosticPacketTypes() {
                return { ionet::PacketType::Metadata_State_Path };
            }

            static std::vector<std::string> GetDiagnosticCounterNames() {
                return { "METADATA C" };
            }

            static std::vector<std::string> GetStatelessValidatorNames() {
                return {
                        "MetadataTypeValidator",
                        "MetadataFieldModificationValidator",
                };
            }

            static std::vector<std::string> GetStatefulValidatorNames() {
                return {
                        "ModifyAddressMetadataValidator",
                        "ModifyMosaicMetadataValidator",
                        "ModifyNamespaceMetadataValidator",
                        "MetadataModificationsValidator",
                };
            }

            static std::vector<std::string> GetObserverNames() {
                return {
                        "AddressMetadataValueModificationObserver",
                        "MosaicMetadataValueModificationObserver",
                        "NamespaceMetadataValueModificationObserver",
                        "MetadataPruningObserver"
                };
            }

            static std::vector<std::string> GetPermanentObserverNames() {
                return GetObserverNames();
            }
        };
    }

    DEFINE_PLUGIN_TESTS(MetadataPluginTests, MetadataPluginTraits)
}}
