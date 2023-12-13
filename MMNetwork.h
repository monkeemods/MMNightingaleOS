/*
  MMNetwork.h - Library for Monkee Mods Network
  Created by sasaug
*/
#ifndef MMNetwork_h
#define MMNetwork_h

#include "Arduino.h"
#include <WiFi.h>
#ifdef ESP32
#include <WiFiMulti.h>
#endif
#include <WiFiUdp.h>
#include <ArduinoJson.h>

#define MMNETWORK_VERSION "0.1.0"

#define UDP_LOCAL_PORT 8888
#define UDP_BROADCAST_PORT 8101
#define UDP_BROADCAST_IP ""
#define HTTP_SERVER_PORT 80

#define STATUS_WIFI_OFFLINE 0
#define STATUS_WIFI_CONNECTED 1
#define STATUS_BCAST_PENDING 2
#define STATUS_SERVER_NOTCONNECTED 3
#define STATUS_SERVER_CONNECTED 4

#define NET_TCP 1
#define NET_UDP 2

#define WIFIMODE_NORMAL 0
#define WIFIMODE_AP 1

#ifdef ESP32
  #define UDP_TX_PACKET_MAX_SIZE 128
#endif

//server is not yet live
#define REMOTESERVER_IP "54.151.218.78"
#define REMOTESERVER_PORT 8100

#define PING_LOST_MAX_TIME 15000

const String NETMSG_CLIENT_BCAST = "client;bcast;%1";
const String NETMSG_SERVER_BCAST  = "server;bcast";
const String NETMSG_CLIENT_HANDSHAKE = "client;handshake;%1;%2;%3";
const String NETMSG_SERVER_HANDSHAKE = "server;handshake";
const String NETMSG_SERVER_PING = "server;ping";
const String NETMSG_CLIENT_PING = "client;ping";

//Client specific network message
const int UDP_PORT = 8888; 
const int UDP_BCAST_PORT = UDP_BROADCAST_PORT; 

class MMNetwork{
  public:
	  MMNetwork(String name);
    String getVersion();
	  void setup();
    int loop();
    bool isWiFiConnected();
    int getNetworkStatus();
    bool isServerConnected();
    void sendMessage(String message);
    typedef void (*cbfunc_t) (String message);
    typedef void (*cbfunc_c) ();
    void setServerMessageCallback(cbfunc_t callbackFunction);
    void setServerConnectedCallback(cbfunc_c callbackFunction);

    void setLoginUsername(String username);
    void setLoginPassword(String password);
    void addWiFiSSID(String ssid, String password);
    void setWiFiMode(int mode);
    void setWiFiAPSSID(String ssid);
    void setWiFiAPPassword(String password);

  private:
    bool connectToWiFi(); 
    String readUDPPacket(int packetSize);
    void sendUDPPacket(const char* message);
    void sendBroadcastUDPPacket(const char* message);
    bool connectTCPServer();
    String readTCPMessage();
    void sendTCPMessage(String message, bool waitTimeout);
    cbfunc_t onServerMessageCallback;
    cbfunc_c onServerConnectedCallback;
    

    String name;
    String loginUser = "";
    String loginPassword = "";
    WiFiMulti multi;
    WiFiUDP Udp;
    String wifiMacAddress;
    String udpBroadcastIp;
    IPAddress myLocalIp;
    IPAddress remoteServerIp;
    int remoteServerPort = -1;
    long unsigned udpLastBroadcastTime = 0;
    int udpBroadcastAttempt = 0;
    WiFiClient tcpClient;
    int currentStatus = STATUS_WIFI_OFFLINE;	
    bool isServerMessageCallbackAvailable = false;
    bool isServerConnectedCallbackAvailable = false;
    int networkMode = NET_TCP;
    int wifiMode = WIFIMODE_NORMAL;
    String apWifiSSID = "mm";
    String apWifiPassword = "monkeemods";
    long unsigned lastPingTime = 0;  
    WiFiServer server = WiFiServer(HTTP_SERVER_PORT);
};


#endif