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

#define TEST_CLASS SCM_Sha3_512Tests

	namespace {
		void ReadLine(std::ifstream& stream, std::string& line, uint64_t lineNumber) {
			std::getline(stream, line);
			if (!stream.good() && !stream.eof())
				CATAPULT_THROW_RUNTIME_ERROR_1("read error", lineNumber)

			if (line.size() && line[line.size() - 1] == '\r')
				line.pop_back();
		}
	}

	void TestSha3_512(const std::string& inputFilePath, const std::string& outputFilePath) {
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
			outputStream << line << std::endl;

			if (line.empty())
				continue;

			auto pos = line.find("Len = ");
			if (std::string::npos == pos)
				continue;
			auto subStr = line.substr(pos + std::string("Len = ").size());
			messageLength = boost::lexical_cast<size_t>(subStr);

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

			auto buffer = test::ToVector(message);
			Hash512 hash;
			Sha3_512_CustomLength(buffer, messageLength, hash);

			auto hashStr = test::ToString(hash);
			boost::to_lower(hashStr);
			outputStream << "MD = " << hashStr << std::endl << std::endl;
		}
	}

	TEST(TEST_CLASS, SHA3_512ShortMsg) {
		TestSha3_512("../resources/SHA3_512ShortMsg.req", "../resources/result/SHA3_512ShortMsg.rsp");
	}

	TEST(TEST_CLASS, SHA3_512LongMsg) {
		TestSha3_512("../resources/SHA3_512LongMsg.req", "../resources/result/SHA3_512LongMsg.rsp");
	}

	TEST(TEST_CLASS, SHA3_512MonteCarlo) {
		// Arrange:
		std::ifstream inputStream("../resources/SHA3_512Monte.req");
		std::ofstream outputStream("../resources/result/SHA3_512Monte.rsp");
		uint64_t lineNumber = 0u;
		std::string line;
		std::string message;
		while (inputStream.good()) {
			++lineNumber;

			ReadLine(inputStream, line, lineNumber);
			outputStream << line << std::endl;

			if (line.empty())
				continue;

			auto pos = line.find("Seed = ");
			if (std::string::npos == pos)
				continue;
			outputStream << std::endl;
			message = line.substr(pos + std::string("Seed = ").size());

			for (auto i = 0u; i < 100u; ++i) {
				Hash512 hash;
				for (auto j = 0u; j < 1000u; ++j) {
					auto buffer = test::ToVector(message);
					Sha3_512(buffer, hash);
					message = test::ToString(hash);
				}

				boost::to_lower(message);
				outputStream << "j = " << i << std::endl;
				outputStream << "MD = " << message << std::endl << std::endl;
			}
		}
	}
}}