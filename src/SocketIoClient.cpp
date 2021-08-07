#include <SocketIoClient.h>

const String getEventName(const String msg) {
	return msg.substring(4, msg.indexOf("\"",4));
}

const String getEventPayload(const String msg) {
	String result = msg.substring(msg.indexOf("\"",4)+2,msg.length()-1);
	if(result.startsWith("\"")) {
		result.remove(0,1);
	}
	if(result.endsWith("\"")) {
		result.remove(result.length()-1);
	}
	return result;
}

// static void hexdump(const uint32_t* src, size_t count) {
//     for (size_t i = 0; i < count; ++i) {
//         Serial.printf("%08x ", *src);
//         ++src;
//         if ((i + 1) % 4 == 0) {
//             Serial.printf("\n");
//         }
//     }
// }

void SocketIoClient::handleEventConnect(const String msg) {
	sessionId = msg.substring(9, msg.indexOf("\"",9)).c_str();
	uint8_t start = msg.indexOf("\"",9)+30;
	_pingInterval = (uint16_t)msg.substring(start,msg.indexOf(",",start)).toInt();
	_pingTimeout = (uint16_t)msg.substring(msg.indexOf(",",start)+15, msg.length()-1).toInt();
}

void SocketIoClient::onMessageCallback(WebsocketsMessage message) {
    Serial.print("Got Message: ");
    Serial.println(message.data());

	if (message.isEmpty()) {
		return;
	}
	if (message.isText()) {
		if(message.data().startsWith("42")) {
			trigger(getEventName(message.data()).c_str(), getEventPayload(message.data()).c_str(), getEventPayload(message.data()).length());
		} else if(message.data().startsWith("2")) {
			_webSocket.send("3");
		} else if(message.data().startsWith("40")) {
			trigger("connect", NULL, 0);
		} else if(message.data().startsWith("41")) {
			trigger("disconnect", NULL, 0);
		}
		return;
	}
	if (message.isBinary()) {
		SOCKETIOCLIENT_DEBUG("[SIoC] get binary length: %u\n", message.length());
		//hexdump((const uint32_t*) payload, length);
	}
	if (message.isPong()) {
		SOCKETIOCLIENT_DEBUG("Got a Pong!");
		return;
	}
	if (message.isPing()) {
		SOCKETIOCLIENT_DEBUG("Got a Ping!");
		return;
	}
}

void SocketIoClient::onEventsCallback(WebsocketsEvent event, String data) {
	switch (event) {
		case WebsocketsEvent::ConnectionOpened:
			if(data.startsWith("0")) {
				handleEventConnect(data);
			}
			SOCKETIOCLIENT_DEBUG("[SIoC] Connected to url: %s\n",  data.c_str());
			break;
		case WebsocketsEvent::ConnectionClosed:
			SOCKETIOCLIENT_DEBUG("[SIoC] Disconnected!\n");
			break;
		case WebsocketsEvent::GotPing:
			SOCKETIOCLIENT_DEBUG("Got a Ping!");
			break;
		case WebsocketsEvent::GotPong:
			SOCKETIOCLIENT_DEBUG("Got a Pong!");
			break;
	}
}

void SocketIoClient::begin(const char* host) {
    _webSocket.connect(host);
    initialize();
}

void SocketIoClient::begin(const char* host, const int port, const char* path) {
    _webSocket.connect(host, port, path);
    initialize();
}

void SocketIoClient::beginSSL(const char* host, const char* fingerprint) {
	// Before connecting, set the ssl fingerprint of the server
    _webSocket.setFingerprint(fingerprint);
    _webSocket.connect(host);
    initialize();
}

void SocketIoClient::beginSSL(const char* host, const int port, const char* path, const char* fingerprint) {
	// Before connecting, set the ssl fingerprint of the server
    _webSocket.setFingerprint(fingerprint);
    _webSocket.connect(host, port, path);
    initialize();
}

void SocketIoClient::beginSSLCert(const char* host, const char* ca_cert, const char* client_private_key, const char* client_cert) {
    SOCKETIOCLIENT_DEBUG("Successfully connected to WiFi, setting time... ");

    // We configure ESP8266's time, as we need it to validate the certificates
    configTime(2 * 3600, 1, ntp1, ntp2);
    while(now < 2 * 3600) {
        Serial.print(".");
        delay(500);
        now = time(nullptr);
    }
    SOCKETIOCLIENT_DEBUG("\n");
	SOCKETIOCLIENT_DEBUG("Time set, connecting to server...");

    // Before connecting, set the ssl certificates and key of the server
    X509List cert(ca_cert);
    _webSocket.setTrustAnchors(&cert);

    X509List *serverCertList = new X509List(client_cert);
    PrivateKey *serverPrivKey = new PrivateKey(client_private_key);
    _webSocket.setClientRSACert(serverCertList, serverPrivKey);

    _webSocket.connect(host);
    initialize();
}

void SocketIoClient::beginSSLCert(const char* host, const int port, const char* path, const char* ca_cert, const char* client_private_key, const char* client_cert) {
    SOCKETIOCLIENT_DEBUG("Successfully connected to WiFi, setting time... ");

    // We configure ESP8266's time, as we need it to validate the certificates
    configTime(2 * 3600, 1, ntp1, ntp2);
    while(now < 2 * 3600) {
        Serial.print(".");
        delay(500);
        now = time(nullptr);
    }
    SOCKETIOCLIENT_DEBUG("\n");
	SOCKETIOCLIENT_DEBUG("Time set, connecting to server...");

    // Before connecting, set the ssl certificates and key of the server
    X509List cert(ca_cert);
    _webSocket.setTrustAnchors(&cert);

    X509List *serverCertList = new X509List(client_cert);
    PrivateKey *serverPrivKey = new PrivateKey(client_private_key);
    _webSocket.setClientRSACert(serverCertList, serverPrivKey);

    _webSocket.connect(host, port, path);
    initialize();
}

void SocketIoClient::initialize() {
    // run callback when messages are received
    _webSocket.onMessage(std::bind(&SocketIoClient::onMessageCallback, this, std::placeholders::_1));
    // run callback when events are occuring
    _webSocket.onEvent(std::bind(&SocketIoClient::onEventsCallback, this, std::placeholders::_1, std::placeholders::_2));
    //_webSocket.onEvent(std::bind(&SocketIoClient::webSocketEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	_lastPing = millis();
}

void SocketIoClient::loop() {
	_webSocket.poll();
	for(auto packet=_packets.begin(); packet != _packets.end();) {
		if(_webSocket.send(*packet)) {
			SOCKETIOCLIENT_DEBUG("[SIoC] packet \"%s\" emitted\n", packet->c_str());
			packet = _packets.erase(packet);
		} else {
			++packet;
		}
	}

	if(millis() - _lastPing > _pingInterval) {
		_webSocket.ping();
		_lastPing = millis();
	}
}

void SocketIoClient::on(const char* event, std::function<void (const char * payload, size_t length)> func) {
	_events[event] = func;
}

void SocketIoClient::emit(const char* event, const char * payload) {
	String msg = String("42[\"");
	msg += event;
	msg += "\"";
	if(payload) {
		msg += ",\"";
		msg += payload;
		msg += "\"";
	}
	msg += "]";
	SOCKETIOCLIENT_DEBUG("[SIoC] add packet %s\n", msg.c_str());
	_packets.push_back(msg);
}

void SocketIoClient::remove(const char* event) {
	auto e = _events.find(event);
	if(e != _events.end()) {
		_events.erase(e);
	} else {
		SOCKETIOCLIENT_DEBUG("[SIoC] event %s not found, can not be removed", event);
	}
}

void SocketIoClient::trigger(const char* event, const char * payload, size_t length) {
	auto e = _events.find(event);
	if(e != _events.end()) {
		SOCKETIOCLIENT_DEBUG("[SIoC] trigger event %s\n", event);
		e->second(payload, length);
	} else {
		SOCKETIOCLIENT_DEBUG("[SIoC] event %s not found. %d events available\n", event, _events.size());
	}
}

void SocketIoClient::disconnect()
{
	_webSocket.close(CloseReason_GoingAway);
	trigger("disconnect", NULL, 0);
}

void SocketIoClient::setAuthorization(const char * user, const char * password) {
	//TODO: to be implemented later
    // _webSocket.setAuthorization(user, password);
}
