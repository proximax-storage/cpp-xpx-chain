/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/plugins/PluginUtils.h"
#include "src/plugins/SdaExchangePlugin.h"
#include "plugins/txes/exchange_sda/src/model/SdaExchangeEntityType.h"
#include "tests/test/plugins/PluginManagerFactory.h"
#include "tests/test/plugins/PluginTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace plugins {

    namespace {
        struct SdaExchangePluginTraits {
        public:
            template<typename TAction>
            static void RunTestAfterRegistration(TAction action) {
                // Arrange:
                auto config = model::NetworkConfiguration::Uninitialized();
                config.Plugins.emplace(PLUGIN_NAME(exchangesda), utils::ConfigurationBag({{
                    "",
                    {
                        { "enabled", "true" },
                        { "maxOfferDuration", "1000" },
                        { "longOfferKey", "CFC31B3080B36BC3D59DF4AB936AC72F4DC15CE3C3E1B1EC5EA41415A4C33FEE" },
                        { "offerSortPolicy", "1"},
                    }
                }}));

                auto manager = test::CreatePluginManager(config);
                RegisterSdaExchangeSubsystem(manager);

                // Act:
                action(manager);
            }

        public:
            static std::vector<model::EntityType> GetTransactionTypes() {
                return {
                    model::Entity_Type_Place_Sda_Exchange_Offer,
                    model::Entity_Type_Remove_Sda_Exchange_Offer,
                };
            }

            static std::vector<std::string> GetCacheNames() {
                return { "ExchangeSdaCache", "SdaOfferGroupCache" };
            }

            static std::vector<ionet::PacketType> GetDiagnosticPacketTypes() {
                return { ionet::PacketType::SdaExchange_Infos, ionet::PacketType::SdaOfferGroup_Infos };
            }

            static std::vector<ionet::PacketType> GetNonDiagnosticPacketTypes() {
                return { ionet::PacketType::SdaExchange_State_Path, ionet::PacketType::SdaOfferGroup_State_Path };
            }

            static std::vector<std::string> GetDiagnosticCounterNames() {
                return { "EXCHANGESDA C", "SDA GR C" };
            }

            static std::vector<std::string> GetStatelessValidatorNames() {
                return {
                    "SdaExchangePluginConfigValidator",
                };
            }

            static std::vector<std::string> GetStatefulValidatorNames() {
                return {
                    "PlaceSdaExchangeOfferV1Validator",
                    "RemoveSdaExchangeOfferV1Validator",
                };
            }

            static std::vector<std::string> GetObserverNames() {
                return {
                    "CleanupSdaOffersObserver",
                    "PlaceSdaExchangeOfferV1Observer",
                    "RemoveSdaExchangeOfferV1Observer",
                };
            }

            static std::vector<std::string> GetPermanentObserverNames() {
                return GetObserverNames();
            }
        };
    }

    DEFINE_PLUGIN_TESTS(SdaExchangePluginTests, SdaExchangePluginTraits)
}}
