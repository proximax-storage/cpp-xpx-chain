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

#include "FileTransactionStatusStorage.h"
#include "catapult/io/EntityIoUtils.h"
#include "catapult/model/Transaction.h"
#include <mutex>
#include <shared_mutex>

namespace catapult { namespace filespooling {

	namespace {
		class FileTransactionStatusStorage final : public subscribers::TransactionStatusSubscriber {
		public:
			explicit FileTransactionStatusStorage(std::unique_ptr<io::OutputStream>&& pOutputStream)
					: m_pOutputStream(std::move(pOutputStream))
			{}

		public:
			void notifyStatus(const model::Transaction& transaction, const Height& height, const Hash256& hash, uint32_t status) override {
				// synchronize access because notifyStatus can be called concurrently
				std::unique_lock lock(m_mutex);

				m_pOutputStream->write(hash);
				io::Write64(*m_pOutputStream, height.unwrap());
				io::Write32(*m_pOutputStream, status);
				io::WriteEntity(*m_pOutputStream, transaction);
			}

			void flush() override {
				// synchronize access because flush can be called concurrently by block and transaction dispatchers
				std::unique_lock lock(m_mutex);
				m_pOutputStream->flush();
			}

		private:
			std::unique_ptr<io::OutputStream> m_pOutputStream;
			std::shared_mutex m_mutex;
		};
	}

	std::unique_ptr<subscribers::TransactionStatusSubscriber> CreateFileTransactionStatusStorage(
			std::unique_ptr<io::OutputStream>&& pOutputStream) {
		return std::make_unique<FileTransactionStatusStorage>(std::move(pOutputStream));
	}
}}
