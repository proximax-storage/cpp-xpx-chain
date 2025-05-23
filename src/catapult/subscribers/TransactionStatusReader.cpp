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

#include "TransactionStatusReader.h"
#include "TransactionStatusSubscriber.h"
#include "catapult/io/EntityIoUtils.h"
#include "catapult/io/Stream.h"
#include "catapult/model/Transaction.h"

namespace catapult { namespace subscribers {

	void ReadNextTransactionStatus(io::InputStream& inputStream, TransactionStatusSubscriber& subscriber) {
		Hash256 hash;
		inputStream.read(hash);
		auto height = Height(io::Read64(inputStream));
		auto status = io::Read32(inputStream);
		auto pTransaction = io::ReadEntity<model::Transaction>(inputStream);
		subscriber.notifyStatus(*pTransaction, height, hash, status);
	}
}}
