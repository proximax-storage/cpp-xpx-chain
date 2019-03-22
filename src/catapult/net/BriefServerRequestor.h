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

#pragma once
#include "NodeRequestResult.h"
#include "PeerConnectCode.h"
#include "ServerConnector.h"
#include "catapult/ionet/Node.h"
#include "catapult/ionet/PacketSocket.h"
#include "catapult/thread/Future.h"
#include "catapult/thread/IoThreadPool.h"
#include "catapult/thread/TimedCallback.h"
#include "catapult/utils/HexFormatter.h"

namespace catapult { namespace net {

	namespace detail {
		/// A default compatibility checker that indicates all responses are compatible.
		class AlwaysCompatibleResponseCompatibilityChecker {
		public:
			template<typename T>
			constexpr bool isResponseCompatible(const ionet::Node&, const T&) const {
				return true;
			}
		};
	}

	/// Establishes timed, short-term connections with external nodes for single requests that this (local) node initiates.
	/// \note Connections are ephemeral and intended to be used for a single request then response operation.
	template<typename TRequestPolicy, typename TResponseCompatibilityChecker = detail::AlwaysCompatibleResponseCompatibilityChecker>
	class BriefServerRequestor : public std::enable_shared_from_this<BriefServerRequestor<TRequestPolicy, TResponseCompatibilityChecker>> {
	public:
		/// Remote node response type.
		using ResponseType = typename TRequestPolicy::ResponseType;

		/// Request completion callback type.
		using CallbackType = consumer<NodeRequestResult, const ResponseType&>;

	private:
		// region NodeRequest

		class NodeRequest {
		public:
			NodeRequest(
					const ionet::Node& requestNode,
					const std::shared_ptr<thread::IoThreadPool>& pPool,
					const TResponseCompatibilityChecker& compatibilityChecker,
					const CallbackType& callback)
					: m_requestNode(requestNode)
					, m_pPool(pPool)
					, m_compatibilityChecker(compatibilityChecker)
					, m_callback(callback)
			{}

		public:
			void setTimeout(const utils::TimeSpan& timeout, const std::shared_ptr<ionet::PacketSocket>& pSocket) {
				auto pTimedCallback = thread::MakeTimedCallback(
						m_pPool->ioContext(),
						m_callback,
						NodeRequestResult::Failure_Timeout,
						ResponseType());
				pTimedCallback->setTimeout(timeout);
				pTimedCallback->setTimeoutHandler([pSocket, requestNode = m_requestNode]() {
					pSocket->close();
					CATAPULT_LOG(debug) << TRequestPolicy::Friendly_Name << " request connection to '" << requestNode << "' timed out";
				});
				m_callback = [pTimedCallback](auto result, const auto& response) {
					pTimedCallback->callback(result, response);
				};
			}

		public:
			void complete(PeerConnectCode connectCode) {
				CATAPULT_LOG(debug)
						<< TRequestPolicy::Friendly_Name << " request connection to '" << m_requestNode
						<< "' failed: " << connectCode;
				complete(NodeRequestResult::Failure_Connection);
			}

			void complete(thread::future<ResponseType>&& responseFuture) {
				try {
					const auto& response = responseFuture.get();
					if (!m_compatibilityChecker.isResponseCompatible(m_requestNode, response))
						return complete(NodeRequestResult::Failure_Incompatible);

					complete(response);
				} catch (const catapult_runtime_error& e) {
					CATAPULT_LOG(warning)
							<< "exception thrown during " << TRequestPolicy::Friendly_Name << " request to '"
							<< m_requestNode << "': " << e.what();
					complete(NodeRequestResult::Failure_Interaction);
				}
			}

		private:
			void complete(NodeRequestResult result) {
				m_callback(result, ResponseType());
			}

			void complete(const ResponseType& response) {
				m_callback(NodeRequestResult::Success, response);
			}

		private:
			ionet::Node m_requestNode;
			std::shared_ptr<thread::IoThreadPool> m_pPool;
			TResponseCompatibilityChecker m_compatibilityChecker;
			CallbackType m_callback;
		};

		// endregion

	public:
		/// Creates a server requestor for a server with a key pair of \a keyPair using \a pPool and configured with \a settings
		/// and a custom response compatibility checker (\a responseCompatibilityChecker).
		BriefServerRequestor(
				const std::shared_ptr<thread::IoThreadPool>& pPool,
				const crypto::KeyPair& keyPair,
				const ConnectionSettings& settings,
				const TResponseCompatibilityChecker& responseCompatibilityChecker)
				: m_pPool(pPool)
				, m_responseCompatibilityChecker(responseCompatibilityChecker)
				, m_requestTimeout(settings.Timeout)
				, m_pConnector(CreateServerConnector(pPool, keyPair, settings))
				, m_numTotalRequests(0)
				, m_numSuccessfulRequests(0)
		{}

	public:
		/// Gets the number of active connections.
		size_t numActiveConnections() const {
			return m_pConnector->numActiveConnections();
		}

		/// Gets the number of total requests.
		size_t numTotalRequests() const {
			return m_numTotalRequests;
		}

		/// Gets the number of successful requests.
		size_t numSuccessfulRequests() const {
			return m_numSuccessfulRequests;
		}

	public:
		/// Initiates a request for data from \a node and calls \a callback on completion.
		void beginRequest(const ionet::Node& node, const CallbackType& callback) {
			++m_numTotalRequests;
			auto wrappedCallback = [pThis = this->shared_from_this(), callback](auto result, const auto& response) {
				if (NodeRequestResult::Success == result)
					++pThis->m_numSuccessfulRequests;

				callback(result, response);
			};

			auto pRequest = std::make_shared<NodeRequest>(node, m_pPool, m_responseCompatibilityChecker, wrappedCallback);
			m_pConnector->connect(node, [pRequest, requestTimeout = m_requestTimeout](auto connectCode, const auto& pSocket) {
				pRequest->setTimeout(requestTimeout, pSocket);

				if (PeerConnectCode::Accepted != connectCode)
					return pRequest->complete(connectCode);

				TRequestPolicy::CreateFuture(*pSocket).then([pSocket, pRequest](auto&& responseFuture) {
					pRequest->complete(std::move(responseFuture));
				});
			});
		}

		/// Shuts down all connections.
		void shutdown() {
			m_pConnector->shutdown();
		}

	private:
		std::shared_ptr<thread::IoThreadPool> m_pPool;
		TResponseCompatibilityChecker m_responseCompatibilityChecker;
		utils::TimeSpan m_requestTimeout;
		std::shared_ptr<ServerConnector> m_pConnector;

		std::atomic<size_t> m_numTotalRequests;
		std::atomic<size_t> m_numSuccessfulRequests;
	};

	/// Initiates a request for data from \a node using \a requestor and returns a future.
	template<typename TRequestor, typename TResult = std::pair<NodeRequestResult, typename TRequestor::ResponseType>>
	thread::future<TResult> BeginRequestFuture(TRequestor& requestor, const ionet::Node& node) {
		auto pPromise = std::make_shared<thread::promise<TResult>>();

		requestor.beginRequest(node, [pPromise](auto result, const auto& response) {
			pPromise->set_value(std::make_pair(result, response));
		});

		return pPromise->get_future();
	}
}}
