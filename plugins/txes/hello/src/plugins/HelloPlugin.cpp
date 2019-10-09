/**
*** FOR TRAINING PURPOSES ONLY
**/

#include "HelloPlugin.h"
#include "HelloTransactionPlugin.h"
#include "src/config/HelloConfiguration.h"
#include "src/validators/Validators.h"
#include "catapult/plugins/PluginManager.h"
#include "src/observers/Observers.h"
#include "src/cache/HelloCache.h"
#include "src/cache/HelloCacheStorage.h"
#include "catapult/plugins/CacheHandlers.h"

namespace catapult { namespace plugins {

        void RegisterHelloSubsystem(PluginManager& manager) {

            //-- created via call to DEFINE_TRANSACTION_PLUGIN_FACTORY in HelloTransactionPlugin.cpp
            manager.addTransactionSupport(CreateHelloTransactionPlugin());

            manager.addStatelessValidatorHook([](auto& builder) {
                builder
                        .add(validators::CreateHelloPluginConfigValidator());
            });

            const auto& pConfigHolder = manager.configHolder();
            manager.addStatefulValidatorHook([pConfigHolder](auto& builder) {
                builder
                        .add(validators::CreateHelloMessageCountValidator(pConfigHolder)); // created in Validators.h using macro DECLARE_STATEFUL_VALIDATOR
            });

            manager.addCacheSupport<cache::HelloCacheStorage>(
                    std::make_unique<cache::HelloCache>(manager.cacheConfig(cache::HelloCache::Name)));

            using CacheHandlersHello = CacheHandlers<cache::HelloCacheDescriptor>;
            CacheHandlersHello::Register<model::FacilityCode::Hello>(manager);

            manager.addObserverHook([](auto& builder) {
                builder
                        .add(observers::CreateHelloObserver());
            });

            manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
                counters.emplace_back(utils::DiagnosticCounterId("HELLO C"), [&cache]() {
                    return cache.sub<cache::HelloCache>().createView(cache.height())->size();
                });
            });

        }
    }}

extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
    catapult::plugins::RegisterHelloSubsystem(manager);
}
