#include <tdme/engine/logics/ApplicationServerClient.h>

#include <string>
#include <vector>

#include <tdme/tdme.h>
#include <tdme/engine/logics/LogicNetworkPacket.h>
#include <tdme/network/udp/UDPPacket.h>
#include <tdme/network/udpserver/UDPServer.h>
#include <tdme/network/udpserver/UDPServerClient.h>
#include <tdme/utilities/Console.h>
#include <tdme/utilities/Exception.h>

using std::endl;
using std::string;
using std::to_string;
using std::vector;

using tdme::engine::logics::LogicNetworkPacket;
using tdme::network::udp::UDPPacket;
using tdme::network::udpserver::UDPServer;
using tdme::network::udpserver::UDPServerClient;
using tdme::utilities::Console;
using tdme::utilities::Exception;

using tdme::engine::logics::ApplicationServerClient;

ApplicationServerClient::ApplicationServerClient(const uint32_t clientId, const string& ip, const unsigned int port) :
	UDPServerClient(clientId, ip, port), networkPacketsMutex("wsserverclient-networkpacketsmutex") {
}

ApplicationServerClient::~ApplicationServerClient() {
	Console::println("ApplicationServerClient::~ApplicationServerClient()");
}

void ApplicationServerClient::onRequest(const UDPPacket* packet, const uint32_t messageId, const uint8_t retries) {
	// check if safe message
	auto safe = packet->getBool();
	if (safe == true && processSafeMessage(messageId) == false) {
		return;
	}
	//
	networkPacketsMutex.lock();
	// while having datas
	while (packet->getPosition() < UDPPacket::PACKET_MAX_SIZE) {
		auto size = packet->getByte();
		// end of
		if (size == 0) break;
		// game logic type id
		auto logicTypeId = packet->getInt();
		// create network packet
		LogicNetworkPacket gameNetworkPacket(
			messageId,
			safe,
			retries,
			logicTypeId,
			packet,
			size
		);
		gameNetworkPacket.setSender(getKey());
		networkPackets.push_back(gameNetworkPacket);
	}
	networkPacketsMutex.unlock();
}

void ApplicationServerClient::onInit() {
	Console::println("Initiated connection with '" + getIp() + ":" + to_string(getPort()) + "', sending key");
}

void ApplicationServerClient::onClose() {
	Console::println("Closed connection with '" + getIp() + ":" + to_string(getPort()) + "'");
}

void ApplicationServerClient::onCustom(const string& type) {
	Console::println("Custom event '" + type + "' with '" + getIp() + ":" + to_string(getPort()) + "'");
}

Mutex& ApplicationServerClient::getNetworkPacketsMutex() {
	return networkPacketsMutex;
}

vector<LogicNetworkPacket>& ApplicationServerClient::getNetworkPackets() {
	return networkPackets;
}
