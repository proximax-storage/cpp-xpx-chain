/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma GCC diagnostic error "-Wmissing-field-initializers"

#include "ExecutorService.h"
#include "catapult/extensions/ServiceLocator.h"
#include "catapult/extensions/ServiceState.h"
#include "catapult/io/BlockStorageCache.h"
#include "catapult/thread/MultiServicePool.h"

#include <map>

namespace catapult { namespace contract {

    // region - executor service registrar

    namespace {
        constexpr auto Service_Name = "executor";

        class ExecutorServiceRegistrar : public extensions::ServiceRegistrar {
        public:
        	explicit ExecutorServiceRegistrar(std::shared_ptr<ExecutorService> pExecutorService)
        	: m_pExecutorService(std::move(pExecutorService)) {}

            extensions::ServiceRegistrarInfo info() const override {
                return {"Executor", extensions::ServiceRegistrarPhase::Post_Range_Consumers};
            }

            void registerServiceCounters(extensions::ServiceLocator&) override {
                // no additional counters
            }

            void registerServices(extensions::ServiceLocator& locator, extensions::ServiceState& state) override {
                locator.registerRootedService(Service_Name, m_pExecutorService);

                m_pExecutorService->setServiceState(&state);

                auto& contractState = state.pluginManager().contractState();

                if (contractState.isExecutorRegistered(m_pExecutorService->executorKey())) {
                    CATAPULT_LOG(debug) << "starting executor service";
                    m_pReplicatorService->start();
                }

                m_pReplicatorService.reset();
            }

        private:
            std::shared_ptr<ExecutorService> m_pExecutorService;
        };
    }

    DECLARE_SERVICE_REGISTRAR(Executor)(std::shared_ptr<ExecutorService> pExecutorService) {
    	return std::make_unique<ExecutorServiceRegistrar>(std::move(pExecutorService));
    }

    // endregion

    // region - replicator service implementation

    class ExecutorService::Impl : public std::enable_shared_from_this<ExecutorService::Impl> {
    public:
        Impl(const crypto::KeyPair& keyPair,
             extensions::ServiceState& serviceState,
             state::ContractState& contractState)
             : m_keyPair(keyPair)
             , m_serviceState(serviceState)
             , m_contractState(contractState)
		 {}

    public:
        void start(const StorageConfiguration& storageConfig) {
			const auto& config = m_serviceState.config();
            TransactionSender transactionSender(
				m_keyPair,
				config.Immutable,
				storageConfig,
				m_serviceState.hooks().transactionRangeConsumerFactory()(disruptor::InputSource::Local),
				m_storageState);

			auto pool = m_serviceState.pool().pushIsolatedPool("StorageQuery", 1);

            m_pReplicatorEventHandler = CreateReplicatorEventHandler(
				std::move(pool),
				std::move(transactionSender),
				m_storageState,
				m_transactionStatusHandler,
				m_keyPair);

			std::vector<sirius::drive::ReplicatorInfo> bootstrapReplicators;
			bootstrapReplicators.reserve(m_bootstrapReplicators.size());
			for (const auto& node : m_bootstrapReplicators) {
				boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address(node.endpoint().Host), node.endpoint().Port);
				bootstrapReplicators.emplace_back(sirius::drive::ReplicatorInfo{ endpoint, node.identityKey().array() });
			}

			if (storageConfig.UseRpcReplicator) {
				gHandleLostConnection = storageConfig.RpcHandleLostConnection;
				gDbgRpcChildCrash = storageConfig.RpcDbgChildCrash;
				m_pReplicator = sirius::drive::createRpcReplicator(
						std::string(storageConfig.RpcHost),
						std::stoi(storageConfig.RpcPort),
						reinterpret_cast<const sirius::crypto::KeyPair&>(m_keyPair), // TODO: pass private key string.
						std::string(storageConfig.Host), // TODO: do not use move semantics.
						std::string(storageConfig.Port), // TODO: do not use move semantics.
						std::string(storageConfig.StorageDirectory), // TODO: do not use move semantics.
						std::string(storageConfig.SandboxDirectory), // TODO: do not use move semantics.
						bootstrapReplicators,
						storageConfig.UseTcpSocket,
						*m_pReplicatorEventHandler, // TODO: pass unique_ptr instead of ref.
						nullptr,
						Service_Name);
			}
			else {
				m_pReplicator = sirius::drive::createDefaultReplicator(
						reinterpret_cast<const sirius::crypto::KeyPair&>(m_keyPair), // TODO: pass private key string.
						std::string(storageConfig.Host), // TODO: do not use move semantics.
						std::string(storageConfig.Port), // TODO: do not use move semantics.
						std::string(storageConfig.StorageDirectory), // TODO: do not use move semantics.
						std::string(storageConfig.SandboxDirectory), // TODO: do not use move semantics.
						bootstrapReplicators,
						storageConfig.UseTcpSocket,
						*m_pReplicatorEventHandler, // TODO: pass unique_ptr instead of ref.
						nullptr,
						Service_Name);
			}

			m_pReplicatorEventHandler->setReplicator(m_pReplicator);
			m_pReplicator->start();

            auto drives = m_storageState.getReplicatorDrives(m_keyPair.publicKey());
            for (const auto& drive: drives) {
                addDrive(drive.Id);
            }

			updateReplicatorDownloadChannels();

			m_pReplicator->asyncInitializationFinished();
        }

        void stop() {
			m_pReplicator.reset();
        }

        bool isAlive() {
        	return !m_pReplicator->isConnectionLost();
        }

    private:
        const crypto::KeyPair& m_keyPair;
        extensions::ServiceState& m_serviceState;
        state::StorageState& m_storageState;

        std::shared_ptr<sirius::drive::Replicator> m_pReplicator;
        std::unique_ptr<ReplicatorEventHandler> m_pReplicatorEventHandler;
        TransactionStatusHandler m_transactionStatusHandler;
		const std::vector<ionet::Node>& m_bootstrapReplicators;

		// The fields are needed to generate correct events
		std::map<Key, Height> m_alreadyAddedDrives;
		std::map<Hash256, ShortAddedChannelInfo> m_alreadyAddedChannels;
    };

    // endregion

    // region - replicator service

	ExecutorService::ExecutorService(ExecutorConfiguration&& executorConfig)
		: m_keyPair(crypto::KeyPair::FromString(executorConfig.Key))
		, m_executorConfig(std::move(executorConfig)) {}

	ExecutorService::~ExecutorService() {
		stop();
	}

	void ExecutorService::start() {
        if (m_pImpl)
			CATAPULT_THROW_RUNTIME_ERROR("replicator service already started");

        m_pImpl = std::make_unique<ReplicatorService::Impl>(
			m_keyPair,
			*m_pServiceState,
			m_pServiceState->pluginManager().storageState(),
			m_bootstrapReplicators);
        m_pImpl->start(m_storageConfig);
    }

    void ReplicatorService::stop() {
        if (m_pImpl) {
            m_pImpl->stop();
            m_pImpl.reset();
        }
    }

	void ReplicatorService::restart() {
		stop();
		sleep(10);
		start();
	}

	void ReplicatorService::maybeRestart() {
		if (m_pImpl && !m_pImpl->isAlive()) {
			restart();
		}
	}

	void ExecutorService::setServiceState(extensions::ServiceState* pServiceState) {
		m_pServiceState = pServiceState;
	}

	const Key& ExecutorService::executorKey() const {
        return m_keyPair.publicKey();
    }

    bool ExecutorService::isExecutorRegistered(const Key& key) {
		// TODO
        return false;
    }

	// endregion
}}
