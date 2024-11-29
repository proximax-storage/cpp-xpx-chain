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

#include "catapult/io/FileBlockStorage.h"
#include "catapult/io/Stream.h"
#include "NemesisTransactions.h"

namespace catapult {
	namespace model { struct BlockElement; }
	namespace tools { namespace nemgen { struct NemesisConfiguration; } }
}

namespace catapult { namespace tools { namespace nemgen {

	/// Saves nemesis \a blockElement according to \a config.
	void SaveNemesisBlockElement(const model::BlockElement& blockElement, const NemesisConfiguration& config);

	/// Writes \a blockElement into \a outputStream.
	void WriteBlockElement(io::OutputStream& outputStream, const model::BlockElement& blockElement, NemesisTransactions& transactions);

	/// Saves nemesis \a blockElement according to \a config.
	void SaveNemesisBlockElementWithSpooling(const model::BlockElement& blockElement, const NemesisConfiguration& config, NemesisTransactions& transactions);

	class NemesisFileBlockStorage : public io::FileBlockStorage {
	/// Creates a file-based block storage, where blocks will be stored inside \a dataDirectory
	/// with specified storage \a mode.
	explicit NemesisFileBlockStorage(const std::string& dataDirectory, io::FileBlockStorageMode mode = io::FileBlockStorageMode::Hash_Index) : FileBlockStorage(dataDirectory, mode) {
		
	}
	void saveBlock(const model::BlockElement& blockElement, NemesisTransactions& transactions);
};
}}}
