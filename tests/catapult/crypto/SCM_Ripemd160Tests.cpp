/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/crypto/Hashes.h"
#include "tests/TestHarness.h"
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <fstream>

namespace catapult { namespace crypto {

#define TEST_CLASS SCM_Ripemd160Tests

	namespace {
		void ReadLine(std::ifstream& stream, std::string& line, uint64_t lineNumber) {
			std::getline(stream, line);
			if (!stream.good() && !stream.eof())
				CATAPULT_THROW_RUNTIME_ERROR_1("read error", lineNumber)
		}
	}

	void TestRipemd160(const std::string& inputFilePath, const std::string& outputFilePath) {
		// Arrange:
		std::ifstream inputStream(inputFilePath);
		std::ofstream outputStream(outputFilePath);
		uint64_t lineNumber = 0u;
		std::string line;
		uint32_t messageLength = 0u;
		std::string message;
		while (inputStream.good()) {
			++lineNumber;

			ReadLine(inputStream, line, lineNumber);

			if (line.empty())
				continue;

			auto pos = line.find("Len");
			if (std::string::npos == pos)
				CATAPULT_THROW_RUNTIME_ERROR_2("invalid line", lineNumber, line)
			outputStream << line << std::endl;
			auto subStr = line.substr(pos + std::string("Len = ").size());
			messageLength = boost::lexical_cast<uint32_t>(subStr);

			ReadLine(inputStream, line, lineNumber);
			outputStream << line << std::endl;
			pos = line.find("Msg");
			if (std::string::npos == pos)
				CATAPULT_THROW_RUNTIME_ERROR_2("invalid line", lineNumber, line)
			if (messageLength) {
				message = line.substr(pos + std::string("Msg = ").size());
			} else {
				message.clear();
			}

			ReadLine(inputStream, line, lineNumber);
			pos = line.find("MD");
			if (std::string::npos == pos)
				CATAPULT_THROW_RUNTIME_ERROR_2("invalid line", lineNumber, line)

			auto buffer = test::ToVector(message);
			Hash160 hash;
			Ripemd160(buffer, hash);

			auto hashStr = test::ToString(hash);
			boost::to_lower(hashStr);
			outputStream << "MD = " << hashStr << std::endl << std::endl;
		}
	}

	TEST(TEST_CLASS, RIPEMDLongMsg_Client_ProximaX_V1_2020) {
		TestRipemd160(
			"../resources/RIPEMDLongMsg_Client_ProximaX_V1_2020.txt",
			"../resources/result/RIPEMDLongMsg_Client_ProximaX_V1_2020.txt");
	}

	TEST(TEST_CLASS, RIPEMDShortMsg_Client_ProximaX_V1_2020) {
		TestRipemd160(
			"../resources/RIPEMDShortMsg_Client_ProximaX_V1_2020.txt",
			"../resources/result/RIPEMDShortMsg_Client_ProximaX_V1_2020.txt");
	}
}}