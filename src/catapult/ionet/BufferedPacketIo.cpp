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

#include "BufferedPacketIo.h"
#include "PacketIo.h"
#include <deque>

namespace catapult { namespace ionet {

	namespace {
		class WriteRequest {
		public:
			explicit WriteRequest(PacketPayload payload)
				: m_payload(std::move(payload))
			{}

		public:
			template<typename TCallback>
			void invoke(const std::shared_ptr<PacketIo>& pIo, TCallback callback) {
				pIo->write(m_payload, callback);
			}

		private:
			PacketPayload m_payload;
		};

		class ReadRequest {
		public:
			template<typename TCallback>
			void invoke(const std::shared_ptr<PacketIo>& pIo, TCallback callback) {
				pIo->read(callback);
			}
		};

		// simple queue implementation
		template<typename TRequest, typename TCallback, typename TCallbackWrapper>
		class RequestQueue {
		public:
			explicit RequestQueue(const std::weak_ptr<PacketIo>& pIo, TCallbackWrapper& wrapper)
				: m_pIo(pIo)
				, m_wrapper(wrapper)
			{}

		public:
			void push(const TRequest& request, const TCallback& callback) {
				auto hasPendingWork = !m_requests.empty();
				m_requests.emplace_back(request, callback);

				if (hasPendingWork) {
					CATAPULT_LOG(trace) << "queuing work because in progress operation detected";
					return;
				}

				next();
			}

		private:
			void next() {
				auto pIo = m_pIo.lock();
				if (!pIo) {
					CATAPULT_LOG(trace) << "no packet io, clearing queue";
					m_requests.clear();
					return;
				}

				// note that it's very important to not call pop_front here - the request should only be popped
				// after the callback is invoked (and the operation is complete)
				auto& request = m_requests.front();
				request.first.invoke(pIo, m_wrapper.wrap(WrappedWithRequests<TCallback>(request.second, *this)));
			}

			template<typename THandler>
			struct WrappedWithRequests {
				WrappedWithRequests(THandler handler, RequestQueue& queue)
						: m_handler(std::move(handler))
						, m_queue(queue)
				{}

				template<typename... TArgs>
				void operator()(TArgs ...args) {
					// the queue may be empty because of the packet io destroyed
					if (!m_queue.m_requests.empty()) {
						// pop the current request (the operation has completed)
						m_queue.m_requests.pop_front();
					}

					// execute the user handler
					m_handler(std::forward<TArgs>(args)...);

					// if requests are pending, start the next one
					if (!m_queue.m_requests.empty())
						m_queue.next();
				}

			private:
				THandler m_handler;
				RequestQueue& m_queue;
			};

		private:
			std::weak_ptr<PacketIo> m_pIo;
			TCallbackWrapper& m_wrapper;
			std::deque<std::pair<TRequest, TCallback>> m_requests;
		};

		/// Protects RequestQueue via a strand.
		template<typename TRequest, typename TCallback>
		class QueuedOperation {
		public:
			explicit QueuedOperation(const std::weak_ptr<PacketIo>& pIo, boost::asio::io_context::strand& strand)
					: m_strand(strand)
					, m_requests(pIo, m_strand)
			{}

		public:
			void push(const TRequest& request, const TCallback& callback) {
				boost::asio::post(m_strand, [this, request, callback] {
					m_requests.push(request, callback);
				});
			}

		private:
			boost::asio::io_context::strand& m_strand;
			RequestQueue<TRequest, TCallback, boost::asio::io_context::strand> m_requests;
		};

		using QueuedWriteOperation = QueuedOperation<WriteRequest, PacketIo::WriteCallback>;
		using QueuedReadOperation = QueuedOperation<ReadRequest, PacketIo::ReadCallback>;

		class BufferedPacketIo
				: public PacketIo
				, public std::enable_shared_from_this<BufferedPacketIo> {
		public:
			BufferedPacketIo(const std::weak_ptr<PacketIo>& pIo, boost::asio::io_context::strand& strand)
					: m_pWriteOperation(std::make_unique<QueuedWriteOperation>(pIo, strand))
					, m_pReadOperation(std::make_unique<QueuedReadOperation>(pIo, strand))
			{}

		public:
			void write(const PacketPayload& payload, const WriteCallback& callback) override {
				auto request = WriteRequest(payload);
				m_pWriteOperation->push(request, [pThis = shared_from_this(), callback](auto code) {
					callback(code);
				});
			}

			void read(const ReadCallback& callback) override {
				auto request = ReadRequest();
				m_pReadOperation->push(request, [pThis = shared_from_this(), callback](auto code, const auto* pPacket) {
					callback(code, pPacket);
				});
			}

		private:
			std::unique_ptr<QueuedWriteOperation> m_pWriteOperation;
			std::unique_ptr<QueuedReadOperation> m_pReadOperation;
		};
	}

	std::shared_ptr<PacketIo> CreateBufferedPacketIo(const std::weak_ptr<PacketIo>& pIo, boost::asio::io_context::strand& strand) {
		return std::make_shared<BufferedPacketIo>(pIo, strand);
	}
}}
