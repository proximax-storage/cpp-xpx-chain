/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/builders/TransferBuilder.h"
#include "catapult/crypto/KeyPair.h"
#include "catapult/crypto/KeyUtils.h"
#include "catapult/extensions/TransactionExtensions.h"
#include "catapult/ionet/Node.h"
#include "catapult/ionet/PacketPayloadFactory.h"
#include "catapult/ionet/PacketSocket.h"
#include "catapult/model/Address.h"
#include "catapult/model/Mosaic.h"
#include "catapult/net/VerifyPeer.h"
#include "catapult/utils/FileSize.h"
#include "catapult/utils/HexParser.h"
#include "catapult/utils/NetworkTime.h"
#include "catapult/utils/RawBuffer.h"
#include "catapult/utils/RawBuffer.h"
#include "SpammerOptions.h"
#include "tests/test/core/AddressTestUtils.h"
#include "tests/test/nodeps/Random.h"
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <thread>

using namespace std;
using namespace boost::program_options;
using namespace catapult;
using namespace chrono;

int parseArguments(int argc, const char** argv, SpammerOptions& options) {

	options_description desc("Allowed options");
	desc.add_options()
			("help", "produce help message")
			("host", value<string>(&options.Host)->default_value("127.0.0.1"), "Ip to connect")
			("port", value<unsigned short>(&options.Port)->default_value(3000), "Port to connect")
			("total", value<int>(&options.Total)->default_value(10), "Total count of transactions")
			("rate", value<int>(&options.Rate)->default_value(1), "Rate transactions per second")
			("mode", value<string>(&options.Mode)->default_value("rest"), "Mode of connection: {rest, node}")
			("apiNodePublicKey", value<string>(&options.ApiNodePublicKey)->default_value(""), "Public key of api node")
			("generationHash", value<string>(&options.GenerationHash)->default_value(""), "Generation hash of network");

	variables_map vm;
	store(parse_command_line(argc, argv, desc), vm);
	notify(vm);

	if (vm.count("help")) {
		cout << desc << "\n";
		return -1;
	}

	return 0;
}

uint8_t RandomByte() {
	std::random_device rd;
	std::mt19937_64 gen;
	auto seed = (static_cast<uint64_t>(rd()) << 32) | rd();
	gen.seed(seed);
	return static_cast<uint8_t>(gen());
}

crypto::KeyPair GenerateRandomKeyPair() {
	return crypto::KeyPair::FromPrivate(crypto::PrivateKey::Generate(RandomByte));
}

model::UniqueEntityPtr<model::Transaction> generateTransferTransaction(const GenerationHash& hash) {
	auto signer = GenerateRandomKeyPair();
	model::NetworkIdentifier networkIdentifier = model::NetworkIdentifier::Public_Test;

	builders::TransferBuilder builder(networkIdentifier, signer.publicKey());
	builder.setRecipient(test::GenerateRandomUnresolvedAddress(networkIdentifier));
	string message = "Hello "+ to_string(std::mt19937()());
	builder.setMessage(RawBuffer((const uint8_t* )message.data(), message.size()));

	model::UniqueEntityPtr<model::Transaction> transaction = builder.build();
	transaction->Deadline = Timestamp(60 * 60 * 1000 + utils::NetworkTime().unwrap());
	transaction->Type = model::Entity_Type_Transfer;
	extensions::TransactionExtensions(hash).sign(signer, *transaction);

	return transaction;
}

int sended = 0, got = 0;
char buf[1024];
bool init = true;

void sendRest(const GenerationHash& hash, boost::asio::ip::tcp::socket& sock, const SpammerOptions& options){
	// Generate transaction
	auto pTransaction = generateTransferTransaction(hash);

	auto pPacket = ionet::PacketPayloadFactory::FromEntity(
			ionet::PacketType::Push_Transactions,
			std::shared_ptr<model::Transaction>(std::move(pTransaction))
	);
	const uint8_t* data = pPacket.buffers()[0].pData;
	unsigned long bufferSize = pPacket.buffers()[0].Size;

	std::stringstream jsonStream;
	jsonStream << "{\"payload\":\"";
	for (unsigned long j = 0; j < bufferSize; ++j, ++data) {
		int temp = *data;

		if (!(temp & 0xF0)) {
			jsonStream << std::hex << 0;
		}

		jsonStream << std::hex << temp;
	}
	jsonStream << "\"}";

	string json = jsonStream.str();

	boost::asio::streambuf request;
	std::ostream request_stream(&request);
	request_stream << "PUT " << "/transaction" << " HTTP/1.0\r\n";
	request_stream << "Host: " << options.Host << ":" << options.Port << "\r\n";
	request_stream << "Content-Type: " << "application/json" << "\r\n";
	request_stream << "Content-Length: " << json.size() << "\r\n";
	request_stream << "Connection: Keep-Alive\r\n";
	request_stream << "Keep-Alive: timeout=100, max=100000\r\n\r\n";
	request_stream << json << "\r\n\r\n";

	sock.async_send(boost::asio::buffer(request.data(), request.size()), 0, [&](auto& err, auto&){

		if (err) {
			cout << err << endl;
			return;
		}

		++got;

		std::string response;
		boost::system::error_code ec;
		sock.receive(boost::asio::buffer(buf), {}, ec);
		if (ec) {
			cout << ec << endl;
			return;
		}
		if (init) {
			init = false;
			sendRest(hash, sock, options);
		}
	});

	if (++sended % 500 == 0)
	{
		cout << sended << endl;
	}

	if (!init && sended < options.Total) {
		thread([&](){
			usleep(1000000 / options.Rate);
			sendRest(hash, sock, options);
		}).detach();
	}
}

void sendApi(const shared_ptr<ionet::PacketIo>& pConnectedSocket, const GenerationHash& hash, const SpammerOptions& options) {

	if (sended >= options.Total) {
		return;
	}

	++sended;

	if (sended % 500 == 0)
	{
		cout << sended << endl;
	}
	// Generate transaction
	auto pTransaction = generateTransferTransaction(hash);

	auto pPacket = ionet::PacketPayloadFactory::FromEntity(
			ionet::PacketType::Push_Transactions,
			std::shared_ptr<model::Transaction>(std::move(pTransaction))
	);

	pConnectedSocket->write(pPacket, [pConnectedSocket, &options, &hash](auto code){
		usleep(1000000 / options.Rate);

		if (code != catapult::ionet::SocketOperationCode::Success) {
			cout << code << endl;
		}

		sendApi(pConnectedSocket, hash, options);
		++got;
	});
}

int main(int argc, const char** argv) {
	SpammerOptions options;
	srand(time(0));

	if (parseArguments(argc, argv, options) < 0) {
		return 0;
	}

	cout << fixed << setprecision(4);
	GenerationHash hash = utils::ParseByteArray<GenerationHash>(options.GenerationHash);

	using namespace boost::asio;

	io_service svc;
	ip::tcp::socket sock(svc);

	if (options.Mode == "rest") {
		sock.open(ip::tcp::v4());
		sock.set_option(ip::tcp::no_delay(false));
		sock.set_option(socket_base::keep_alive(true));
		sock.set_option(socket_base::reuse_address(true));
		ip::tcp::endpoint endpoint(ip::address::from_string(options.Host), options.Port);
		sock.connect(endpoint);
		sendRest(hash, sock, options);

		io_service::work some_work(svc);
		svc.run();
	} else if (options.Mode == "node") {

		crypto::KeyPair keyPair = GenerateRandomKeyPair();
		net::VerifiedPeerInfo serverPeerInfo;
		serverPeerInfo.PublicKey = crypto::ParseKey(options.ApiNodePublicKey);
		serverPeerInfo.SecurityMode = ionet::ConnectionSecurityMode::None;

		catapult::ionet::NodeEndpoint endpoint;
		endpoint.Host = options.Host;
		endpoint.Port = options.Port;

		catapult::ionet::PacketSocketOptions packetSocketOptions;
		packetSocketOptions.WorkingBufferSize = catapult::utils::FileSize::FromKilobytes(512).bytes();
		packetSocketOptions.WorkingBufferSensitivity = 100;
		packetSocketOptions.MaxPacketDataSize = catapult::utils::FileSize::FromMegabytes(150).bytes();

		auto cancel = ionet::Connect(
				svc,
				packetSocketOptions,
				endpoint,
				[&](auto, const shared_ptr<ionet::PacketIo>& pConnectedSocket) {
					net::VerifyServer(pConnectedSocket, serverPeerInfo, keyPair, [pConnectedSocket, &options, &hash](auto, const auto&) {
						sendApi(pConnectedSocket, hash, options);
					});
				});

		io_service::work some_work(svc);
		svc.run();
	}

	return 0;
}
