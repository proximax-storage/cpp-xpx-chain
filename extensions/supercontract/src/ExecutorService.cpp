/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ExecutorService.h"
#include "TransactionStatusHandler.h"
#include "StorageBlockchainBuilder.h"
#include "ExecutorEventHandler.h"
#include "catapult/extensions/ServiceLocator.h"
#include "catapult/extensions/ServiceState.h"
#include "catapult/io/BlockStorageCache.h"
#include "catapult/thread/MultiServicePool.h"
#include "catapult/state/ContractState.h"
#include <executor/Executor.h>
#include <executor/DefaultExecutorBuilder.h>
#include <storage/RPCStorageBuilder.h>
#include <messenger/RPCMessengerBuilder.h>
#include <virtualMachine/RPCVirtualMachineBuilder.h>
#include "TransactionSender.h"
#include "ContractLogger.h"
#include <rpcExecutorServer/RPCExecutor.h>
#include "StorageBlockchain.h"

#include <map>

using namespace sirius::contract;

namespace catapult::contract {

	// region - executor service registrar

	namespace {
		constexpr auto Service_Name = "executor";

		class ExecutorServiceRegistrar : public extensions::ServiceRegistrar {
		public:
			explicit ExecutorServiceRegistrar(std::shared_ptr<ExecutorService> pExecutorService)
				: m_pExecutorService(std::move(pExecutorService)) {}

			extensions::ServiceRegistrarInfo info() const override {
				return { "Executor", extensions::ServiceRegistrarPhase::Post_Range_Consumers };
			}

			void registerServiceCounters(extensions::ServiceLocator&) override {
				// no additional counters
			}

			void registerServices(extensions::ServiceLocator& locator, extensions::ServiceState& state) override {
				locator.registerRootedService(Service_Name, m_pExecutorService);

				m_pExecutorService->setServiceState(&state);

				const auto& storage = state.storage();
				auto blockProvider = [&storage](Height height) {
					auto storageView = storage.view();
					return storageView.loadBlockElement(height);
				};

				auto& contractState = state.pluginManager().contractState();
				contractState.setBlockProvider(blockProvider);

				m_pExecutorService->start();

				m_pExecutorService.reset();
			}

		private:
			std::shared_ptr<ExecutorService> m_pExecutorService;
		};
	} // namespace

	DECLARE_SERVICE_REGISTRAR(Executor)(std::shared_ptr<ExecutorService> pExecutorService) {
		return std::make_unique<ExecutorServiceRegistrar>(std::move(pExecutorService));
	}

	// endregion

	// region - replicator service implementation

	class ExecutorService::Impl : public std::enable_shared_from_this<ExecutorService::Impl> {
	public:
		Impl(const crypto::KeyPair& keyPair,
			 extensions::ServiceState& serviceState,
			 const state::ContractState& contractState,
			 const ExecutorConfiguration& config,
			 std::shared_ptr<TransactionStatusHandler> pTransactionStatusHandler)
			: m_keyPair(keyPair)
			, m_serviceState(serviceState)
			, m_contractState(contractState)
			, m_config(config)
			, m_pTransactionStatusHandler(std::move(pTransactionStatusHandler)) {}

	public:
		void start() {
			std::vector<uint8_t> privateKeyBuffer = { m_keyPair.privateKey().begin(), m_keyPair.privateKey().end() };
			auto privateKey = std::move(*reinterpret_cast<sirius::crypto::PrivateKey*>(privateKeyBuffer.data()));
			auto keyPair = sirius::crypto::KeyPair::FromPrivate(std::move(privateKey));

			TransactionSender transactionSender(
					m_keyPair,
					m_serviceState.config().Immutable,
					m_config,
					m_serviceState.hooks().transactionRangeConsumerFactory()(disruptor::InputSource::Local));

			auto pExecutorEventHandler = std::make_shared<ExecutorEventHandler>(
					m_keyPair, std::move(transactionSender), m_pTransactionStatusHandler);

			if (m_config.UseRPCExecutor) {
				auto executor = std::make_unique<sirius::contract::rpcExecutorServer::RPCExecutor>(
						pExecutorEventHandler,
						std::make_shared<sirius::logging::Logger>(std::make_unique<ContractLogger>(), "executor"));
				executor->start(
						m_config.ExecutorRPCHost + ":" + m_config.ExecutorRPCPort,
						std::make_unique<StorageBlockchain>(m_contractState),
						keyPair,
						m_config.StorageRPCHost + ":" + m_config.StorageRPCPort,
						m_config.MessengerRPCHost + ":" + m_config.MessengerRPCPort,
						m_config.VirtualMachineRPCHost + ":" + m_config.VirtualMachineRPCPort,
						m_config.ExecutorLogPath,
						static_cast<uint8_t>(m_serviceState.config().Immutable.NetworkIdentifier));
				m_pExecutor = std::move(executor);
			} else {
				std::unique_ptr<ServiceBuilder<blockchain::Blockchain>> blockchainBuilder =
						std::make_unique<StorageBlockchainBuilder>(m_contractState);

				std::unique_ptr<ServiceBuilder<storage::Storage>> storageBuilder =
						std::make_unique<storage::RPCStorageBuilder>(
								m_config.StorageRPCHost + ":" + m_config.StorageRPCPort);

				std::unique_ptr<messenger::MessengerBuilder> messengerBuilder =
						std::make_unique<messenger::RPCMessengerBuilder>(
								m_config.MessengerRPCHost + ":" + m_config.MessengerRPCPort);

				std::unique_ptr<vm::VirtualMachineBuilder> vmBuilder = std::make_unique<vm::RPCVirtualMachineBuilder>(
						m_config.VirtualMachineRPCHost + ":" + m_config.VirtualMachineRPCPort);

				sirius::contract::ExecutorConfig config;
				config.setNetworkIdentifier(static_cast<uint8_t>(m_serviceState.config().Immutable.NetworkIdentifier));

				m_pExecutor = sirius::contract::DefaultExecutorBuilder().build(
						std::move(keyPair),
						config,
						pExecutorEventHandler,
						std::move(vmBuilder),
						std::move(storageBuilder),
						std::move(blockchainBuilder),
						std::move(messengerBuilder),
						std::make_shared<sirius::logging::Logger>(std::make_unique<ContractLogger>(), "executor"));
			}

			pExecutorEventHandler->setExecutor(m_pExecutor);
		}

	private:
		auto castExecutors(const std::map<Key, state::ExecutorStateInfo>& executors) {
			std::map<sirius::contract::ExecutorKey, sirius::contract::ExecutorInfo> castedExecutors;
			for (const auto& [key, digest] : executors) {
				sirius::contract::ExecutorInfo info;
				info.m_nextBatchToApprove = digest.NextBatchToApprove;
				info.m_batchProof.m_T.fromBytes(digest.PoEx.T.toBytes());
				info.m_batchProof.m_r = digest.PoEx.R.array();
				info.m_initialBatch = digest.PoEx.StartBatchId;
				castedExecutors[key.array()] = info;
			}
			return castedExecutors;
		}

	public:
		std::optional<Height> contractAddedAt(const Key& contractKey) const {
			auto it = m_alreadyAddedContracts.find(contractKey);
			if (it == m_alreadyAddedContracts.end()) {
				return {};
			}
			return it->second;
		}

		void addManualCall(const Key& contractKey, const state::ManualCallInfo& manualCall) {
			std::vector<sirius::contract::ServicePayment> servicePayments;
			for (const auto& payment : manualCall.Payments) {
				servicePayments.push_back(
					{ payment.PaymentUnresolvedMosaicId.unwrap(), payment.PaymentAmount.unwrap() }
				);
			}

			sirius::contract::ManualCallRequest manualCallRequest(
					manualCall.CallId.array(),
					manualCall.FileName,
					manualCall.FunctionName,
					manualCall.ExecutionPayment.unwrap(),
					manualCall.DownloadPayment.unwrap(),
					sirius::contract::CallerKey(manualCall.Caller.array()),
					manualCall.BlockHeight.unwrap(),
					{ manualCall.Arguments.begin(), manualCall.Arguments.end() },
					servicePayments);

			m_pExecutor->addManualCall(contractKey.array(), std::move(manualCallRequest));
		}

		void activateAutomaticExecutions(const Key& contractKey, Height height) {
			auto enabledSince =
					m_contractState.getAutomaticExecutionsEnabledSince(contractKey, height, m_serviceState.config());
			if (enabledSince) {
				m_pExecutor->setAutomaticExecutionsEnabledSince(contractKey.array(), enabledSince->unwrap());
			}
		}

		void successfulBatchExecutionPublished(
				const Key& contractKey,
				uint64_t batchIndex,
				const Hash256& driveState,
				const crypto::CurvePoint& poExVerificationInfo,
				const std::set<Key>& cosigners,
				Height height) {
			batchExecutionPublished(contractKey, batchIndex, true, driveState, poExVerificationInfo, cosigners, height);
		}

		void unsuccessfulBatchExecutionPublished(
				const Key& contractKey,
				uint64_t batchIndex,
				const std::set<Key>& cosigners,
				Height height) {
			auto driveState = m_contractState.getDriveState(contractKey);
			batchExecutionPublished(
					contractKey, batchIndex, false, driveState, crypto::CurvePoint(), cosigners, height);
		}

		void automaticExecutionsBlockPublished(Height height) {
			if (!m_pExecutor) {
				return;
			}

			for (const auto& [contractKey, addedAt] : m_alreadyAddedContracts) {
				if (addedAt < height) {
					m_pExecutor->addBlock(contractKey.array(), height.unwrap());
				}
			}
		}

		void batchExecutionSinglePublished(const Key& contractKey, uint64_t batchIndex) {
			PublishedEndBatchExecutionSingleTransactionInfo publishedTransaction;
			publishedTransaction.m_contractKey = contractKey.array();
			publishedTransaction.m_batchIndex = batchIndex;
			m_pExecutor->onEndBatchExecutionSingleTransactionPublished(std::move(publishedTransaction));
		}

		void synchronizeSinglePublished(const Key& contractKey, uint64_t batchIndex) {
			PublishedSynchronizeSingleTransactionInfo publishedTransaction;
			publishedTransaction.m_contractKey = contractKey.array();
			publishedTransaction.m_batchIndex = batchIndex;
			m_pExecutor->onStorageSynchronizedPublished(std::move(publishedTransaction));
		}

		void updateContracts(Height height) {
			auto actualContracts = m_contractState.getContracts(m_keyPair.publicKey());
			if (!m_pExecutor && !actualContracts.empty()) {
				// lazy start of the executor
				start();
			}

			for (auto it = m_alreadyAddedContracts.begin(); it != m_alreadyAddedContracts.end();) {
				if (actualContracts.find(it->first) == actualContracts.end()) {
					it = m_alreadyAddedContracts.erase(it);
					m_pExecutor->removeContract(it->first.array(), sirius::contract::RemoveRequest {});

					continue;
				}

				++it;
			}

			for (const auto& contractKey : actualContracts) {
				if (m_alreadyAddedContracts.find(contractKey) == m_alreadyAddedContracts.end()) {
					addContract(contractKey, height);
				} else {
					auto executors = m_contractState.getExecutors(contractKey);

					m_pExecutor->setExecutors(contractKey.array(), castExecutors(executors));
				}
			}
		}

		bool contractExists(const Key& contractKey) {
			return m_contractState.contractExists(contractKey);
		}

	private:
		void addContract(const Key& contractKey, const Height& height) {
			m_alreadyAddedContracts.insert({contractKey, height});
			auto contractInfo = m_contractState.getContractInfo(contractKey, height, m_serviceState.config());
			sirius::contract::AddContractRequest addRequest;
			addRequest.m_driveKey = contractInfo.DriveKey.array();
			addRequest.m_executors = castExecutors(contractInfo.Executors);
			addRequest.m_contractDeploymentBaseModificationId = contractInfo.DeploymentBaseModificationId.array();
			addRequest.m_automaticExecutionsFileName = contractInfo.AutomaticExecutionsFileName;
			addRequest.m_automaticExecutionsFunctionName = contractInfo.AutomaticExecutionsFunctionName;
			addRequest.m_automaticExecutionsSCLimit = contractInfo.AutomaticExecutionCallPayment.unwrap();
			addRequest.m_automaticExecutionsSMLimit = contractInfo.AutomaticDownloadCallPayment.unwrap();

			for (const auto& [batchId, verificationInfo] : contractInfo.RecentBatches) {
				sirius::crypto::CurvePoint point;
				point.fromBytes(verificationInfo.toBytes());
				addRequest.m_recentBatchesInformation[batchId] = point;
			}

			m_pExecutor->addContract(contractKey.array(), std::move(addRequest));
			if (contractInfo.LastPublishedBatch) {
				const auto& lastBatch = *contractInfo.LastPublishedBatch;
				PublishedEndBatchExecutionTransactionInfo publishedBatchInfo;
				publishedBatchInfo.m_contractKey = contractKey.array();
				publishedBatchInfo.m_batchIndex = lastBatch.BatchIndex;
				publishedBatchInfo.m_batchSuccess = lastBatch.BatchSuccess;
				publishedBatchInfo.m_driveState = lastBatch.DriveState.array();
				publishedBatchInfo.m_PoExVerificationInfo.fromBytes(lastBatch.PoExVerificationInfo.toBytes());
				publishedBatchInfo.m_cosigners = castInternalType<sirius::contract::ExecutorKey>(lastBatch.Cosigners);
				publishedBatchInfo.m_automaticExecutionsEnabledSince =
						contractInfo.AutomaticExecutionsEnabledSince
								? contractInfo.AutomaticExecutionsEnabledSince->unwrap()
								: std::optional<uint64_t>();
				publishedBatchInfo.m_automaticExecutionsCheckedUpTo =
						contractInfo.AutomaticExecutionsNextBlockToCheck.unwrap();
				m_pExecutor->onEndBatchExecutionPublished(std::move(publishedBatchInfo));
			}

			if (contractInfo.AutomaticExecutionsEnabledSince) {
				m_pExecutor->setAutomaticExecutionsEnabledSince(
						contractKey.array(), contractInfo.AutomaticExecutionsEnabledSince->unwrap());
			}

			std::optional<Height> nextBlockToCheck = contractInfo.AutomaticExecutionsEnabledSince
															 ? std::max(
																	   *contractInfo.AutomaticExecutionsEnabledSince,
																	   contractInfo.AutomaticExecutionsNextBlockToCheck)
															 : std::optional<Height>();
			Height currentBlock(UINT64_MAX);
			if (nextBlockToCheck) {
				currentBlock = *nextBlockToCheck;
			}
			const auto& manualCalls = contractInfo.ManualCalls;
			if (!manualCalls.empty()) {
				currentBlock = std::min(currentBlock, manualCalls.front().BlockHeight);
			}
			auto manualCallsIt = manualCalls.begin();
			while (currentBlock <= height) {
				// heights in manual call always do not exceed @height
				while (manualCallsIt != manualCalls.end() && manualCallsIt->BlockHeight == currentBlock) {
					addManualCall(contractKey, *manualCallsIt);
					manualCallsIt++;
				}
				m_pExecutor->addBlock(contractKey.array(), currentBlock.unwrap());
				currentBlock = currentBlock + Height(1);
			}
		}

		// TODO maybe delete
		void removeContract(const Key& contractKey) {
			m_alreadyAddedContracts.erase(contractKey);
			m_pExecutor->removeContract(contractKey.array(), sirius::contract::RemoveRequest {});
		}

		void batchExecutionPublished(
				const Key& contractKey,
				uint64_t batchIndex,
				bool success,
				const Hash256& driveState,
				const crypto::CurvePoint& poExVerificationInfo,
				const std::set<Key>& cosigners,
				Height height) {
			PublishedEndBatchExecutionTransactionInfo publishedTransaction;
			publishedTransaction.m_contractKey = contractKey.array();
			publishedTransaction.m_batchIndex = batchIndex;
			publishedTransaction.m_batchSuccess = success;
			publishedTransaction.m_driveState = driveState.array();
			publishedTransaction.m_PoExVerificationInfo.fromBytes(poExVerificationInfo.toBytes());
			publishedTransaction.m_cosigners = castInternalType<sirius::contract::ExecutorKey>(cosigners);

			auto automaticExecutionsEnabled =
					m_contractState.getAutomaticExecutionsEnabledSince(contractKey, height, m_serviceState.config());

			auto nextBlockToCheck = m_contractState.getAutomaticExecutionsNextBlockToCheck(contractKey);
			publishedTransaction.m_automaticExecutionsEnabledSince =
					automaticExecutionsEnabled ? automaticExecutionsEnabled->unwrap() : std::optional<uint64_t>();
			publishedTransaction.m_automaticExecutionsCheckedUpTo = nextBlockToCheck.unwrap();

			m_pExecutor->onEndBatchExecutionPublished(std::move(publishedTransaction));
		}

		template<class T, class H>
		std::set<T> castInternalType(const std::set<H>& values) {
			std::set<T> res;
			for (const auto& v : values) {
				res.insert(v.array());
			}

			return res;
		}

	private:
		const crypto::KeyPair& m_keyPair;
		extensions::ServiceState& m_serviceState;
		const state::ContractState& m_contractState;
		const ExecutorConfiguration& m_config;

		std::shared_ptr<TransactionStatusHandler> m_pTransactionStatusHandler;

		// The fields are needed to generate correct events
		std::map<Key, Height> m_alreadyAddedContracts;

		std::shared_ptr<sirius::contract::Executor> m_pExecutor;
	};

	// endregion

	// region - replicator service

	ExecutorService::ExecutorService(
			ExecutorConfiguration&& executorConfig,
			std::shared_ptr<TransactionStatusHandler> pTransactionStatusHandler)
		: m_keyPair(crypto::KeyPair::FromString(executorConfig.Key))
		, m_config(std::move(executorConfig))
		, m_pTransactionStatusHandler(std::move(pTransactionStatusHandler)) {}

	ExecutorService::~ExecutorService() {
		stop();
	}

	void ExecutorService::start() {
		if (m_pImpl) {
			CATAPULT_THROW_RUNTIME_ERROR("executor service already started");
		}

		m_pImpl = std::make_unique<ExecutorService::Impl>(
				m_keyPair,
				*m_pServiceState,
				m_pServiceState->pluginManager().contractState(),
				m_config,
				m_pTransactionStatusHandler);
	}

	void ExecutorService::stop() {
		m_pImpl.reset();
	}

	void ExecutorService::restart() {
		stop();
		sleep(10);
		start();
	}

	void ExecutorService::setServiceState(extensions::ServiceState* pServiceState) {
		m_pServiceState = pServiceState;
	}

	const Key& ExecutorService::executorKey() const {
		return m_keyPair.publicKey();
	}

	std::optional<Height> ExecutorService::contractAddedAt(const Key& contractKey) {
		if (!m_pImpl) {
			return {};
		}
		return m_pImpl->contractAddedAt(contractKey);
	}

	void ExecutorService::addManualCall(
			const Key& contractKey,
			const Hash256& callId,
			const std::string& fileName,
			const std::string& functionName,
			const std::string& parameters,
			Amount executionPayment,
			Amount downloadPayment,
			const Key& caller,
			std::vector<catapult::model::UnresolvedMosaic> servicePayments,
			Height height) {
		if (!m_pImpl) {
			return;
		}

		std::vector<catapult::state::Payment> payments;
		for (const auto& payment : servicePayments) {
			payments.push_back(
				{ payment.MosaicId, payment.Amount }
			);
		}

		m_pImpl->addManualCall(
				contractKey,
				state::ManualCallInfo { callId,
										fileName,
										functionName,
										parameters,
										executionPayment,
										downloadPayment,
										caller,
										height,
										payments });
	}

	void ExecutorService::successfulBatchExecutionPublished(
			const Key& contractKey,
			uint64_t batchIndex,
			const Hash256& driveState,
			const crypto::CurvePoint& poExVerificationInfo,
			const std::set<Key>& cosigners,
			Height height) {
		if (!m_pImpl) {
			return;
		}
		m_pImpl->successfulBatchExecutionPublished(
				contractKey, batchIndex, driveState, poExVerificationInfo, cosigners, height);
	}

	void ExecutorService::unsuccessfulBatchExecutionPublished(
			const Key& contractKey,
			uint64_t batchIndex,
			const std::set<Key>& cosigners,
			Height height) {
		if (!m_pImpl) {
			return;
		}
		m_pImpl->unsuccessfulBatchExecutionPublished(contractKey, batchIndex, cosigners, height);
	}

	void ExecutorService::activateAutomaticExecutions(const Key& contractKey, Height height) {
		if (!m_pImpl) {
			return;
		}
		m_pImpl->activateAutomaticExecutions(contractKey, height);
	}

	bool ExecutorService::contractExists(const Key& contractKey) {
		if (!m_pImpl) {
			return false;
		}
		return m_pImpl->contractExists(contractKey);
	}

	void ExecutorService::automaticExecutionsBlockPublished(Height height) {
		if (!m_pImpl) {
			return;
		}
		m_pImpl->automaticExecutionsBlockPublished(height);
	}

	void ExecutorService::updateContracts(Height height) {
		if (!m_pImpl) {
			return;
		}
		m_pImpl->updateContracts(height);
	}

	void ExecutorService::batchExecutionSinglePublished(const Key& contractKey, uint64_t batchIndex) {
		if (!m_pImpl) {
			return;
		}
		m_pImpl->batchExecutionSinglePublished(contractKey, batchIndex);
	}

	void ExecutorService::synchronizeSinglePublished(const Key& contractKey, uint64_t batchIndex) {
		if (!m_pImpl) {
			return;
		}
		m_pImpl->synchronizeSinglePublished(contractKey, batchIndex);
	}

	// endregion
} // namespace catapult::contract
