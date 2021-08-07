#ifndef __SOCKET_IO_CLIENT_H__
#define __SOCKET_IO_CLIENT_H__

#include <Arduino.h>
#include <map>
#include <vector>
#include <ArduinoWebsockets.h>

#ifndef NODEBUG_SOCKETIOCLIENT
   #define SOCKETIOCLIENT_DEBUG(...) Serial.printf(__VA_ARGS__);
#else
   #define SOCKETIOCLIENT_DEBUG(...)
#endif

//#define SOCKETIOCLIENT_USE_SSL
#ifdef SOCKETIOCLIENT_USE_SSL
	#define DEFAULT_PORT 443
#else
	#define DEFAULT_PORT 80
#endif
#define DEFAULT_PATH "/socket.io/?transport=websocket"
#define DEFAULT_FINGERPRINT ""
#define DEFAULT_CA_CERT ""
#define DEFAULT_CLIENT_PRIVATE_KEY ""
#define DEFAULT_CLIENT_CERT ""

using namespace websockets;

class SocketIoClient {
private:
	std::vector<String> _packets;
	WebsocketsClient _webSocket;
	uint16_t _lastPing;
	uint16_t _pingInterval = 10000;
	uint16_t _pingTimeout = 50000;
	std::map<String, std::function<void (const char * payload, size_t length)>> _events;
	/* NTP Time Servers */
	const char *ntp1 = "time.windows.com";
	const char *ntp2 = "pool.ntp.org";
	const char *sessionId;
	time_t now;

	void trigger(const char* event, const char * payload, size_t length);
	void onMessageCallback(WebsocketsMessage message);
	void onEventsCallback(WebsocketsEvent event, String data);
	void handleEventConnect(const String msg);
	//void webSocketEvent(WebsocketEvent type, uint8_t * payload, size_t length);
    void initialize();
public:
	void begin(const char* host);
	void begin(const char* host, const int port = DEFAULT_PORT, const char* path = DEFAULT_PATH);
    void beginSSL(const char* host, const char* fingerprint = DEFAULT_FINGERPRINT);
    void beginSSL(const char* host, const int port = DEFAULT_PORT, const char* path = DEFAULT_PATH, const char* fingerprint = DEFAULT_FINGERPRINT);
	void beginSSLCert(const char* host, const char* ca_cert = DEFAULT_CA_CERT, const char* client_private_key = DEFAULT_CLIENT_PRIVATE_KEY, const char* client_cert = DEFAULT_CLIENT_CERT);
	void beginSSLCert(const char* host, const int port = DEFAULT_PORT, const char* path = DEFAULT_PATH, const char* ca_cert = DEFAULT_CA_CERT, const char* client_private_key = DEFAULT_CLIENT_PRIVATE_KEY, const char* client_cert = DEFAULT_CLIENT_CERT);
	void loop();
	void on(const char* event, std::function<void (const char * payload, size_t length)>);
	void emit(const char* event, const char * payload = NULL);
	void remove(const char* event);
	void disconnect();
	void setAuthorization(const char * user, const char * password);
};

#endif
