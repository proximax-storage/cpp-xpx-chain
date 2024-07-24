/**
*** Copyright (c) 2016-2019, Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp.
*** Copyright (c) 2020-present, Jaguar0625, gimre, BloodyRookie.
*** All rights reserved.
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

#include "CertificateDirectoryGenerator.h"
#include "CertificateUtils.h"
#include "catapult/config/CatapultDataDirectory.h"
#include "catapult/io/RawFile.h"
#include "catapult/exceptions.h"
#include <filesystem>
#include <random>

namespace catapult { namespace crypto {

	namespace {
		void SaveToFile(const std::string& directory, const std::string& filename, const std::string& buffer) {
			io::RawFile dataFile((std::filesystem::path(directory) / filename).generic_string(), io::OpenMode::Read_Write);
			dataFile.write({ reinterpret_cast<const uint8_t*>(buffer.data()), buffer.size() });
		}

		auto CreateCertificateBuilder(const crypto::KeyPair& keyPair) {
			CertificateBuilder builder;
			builder.setSubject("XD", "CA", "Ca certificate");
			builder.setIssuer("XD", "CA", "Ca certificate");
			builder.setPublicKey(keyPair);
			return builder;
		}

		std::vector<std::shared_ptr<x509_st>> CreateFullCertificateChain(const crypto::KeyPair& nodeKeyPair) {
			auto nodeCertificateBuilder = CreateCertificateBuilder(nodeKeyPair);
			auto pNodeCertificate = nodeCertificateBuilder.buildAndSign(nodeKeyPair);

			return { pNodeCertificate };
		}
	}

	void GenerateCertificateDirectory(const crypto::KeyPair& nodeKeyPair, const std::string& certificateDirectory) {
		if (std::filesystem::exists(certificateDirectory))
			std::filesystem::remove_all(certificateDirectory);

		CATAPULT_LOG(info) << "generating new certificate directory: " << certificateDirectory;
		boost::filesystem::create_directories(certificateDirectory);

		auto certificates = CreateFullCertificateChain(nodeKeyPair);
		auto fullCertificateChain = CreateFullCertificateChainPem(certificates);
		SaveToFile(certificateDirectory, "node.key.pem", CreatePrivateKeyPem(nodeKeyPair));
		SaveToFile(certificateDirectory, "node.full.crt.pem", fullCertificateChain);
	}
}}
