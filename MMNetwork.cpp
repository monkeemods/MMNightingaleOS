#include <WiFi.h>
#include <WiFiUdp.h>
#include "Arduino.h"                                                                                                                             
#include "MMNetwork.h"
#include <ArduinoJson.h>

MMNetwork::MMNetwork(String name){
  this->name = name;
}

String MMNetwork::getVersion(){
  return MMNETWORK_VERSION;
}

void MMNetwork::setup(){
  if(wifiMode == WIFIMODE_AP){
    WiFi.mode(WIFI_AP);
    WiFi.setHostname(apWifiSSID.c_str());
    #ifdef ESP32
      WiFi.softAP(apWifiSSID.c_str(), apWifiPassword.c_str());
    #else
      WiFi.beginAP(apWifiSSID.c_str(), apWifiPassword.c_str());
    #endif
  }else{
    WiFi.mode(WIFI_STA);
  }

  //initialise local UDP Server
  Udp.begin(UDP_LOCAL_PORT);
}

int MMNetwork::loop(){
  if(wifiMode == WIFIMODE_AP){
    return 0;
  }

  //try to connect to wifi
  if (WiFi.status() != WL_CONNECTED) {
    bool status = connectToWiFi();
    if(!status){
        //if connection failed, wait for 5 seconds to retry
        Serial.println("WiFi: Fail to connect, retrying in 5 seconds...");
        delay(5000);
        return 0;
    }else{
      currentStatus = STATUS_WIFI_CONNECTED;
    }
  }

  //stop processing if no login found
  if(loginUser == "" || loginPassword == ""){
    return 0;
  }

  //check if any remote server found, if not we try broadcast a UDP packet
  if(remoteServerPort == -1 && udpLastBroadcastTime <= millis()){
      if(udpBroadcastAttempt < 3){
        //no remote server found yet, broadcast UDP packet to the network
        String msg = NETMSG_CLIENT_BCAST;
        msg.replace("%1", wifiMacAddress);
        sendBroadcastUDPPacket(msg.c_str());

        currentStatus = STATUS_BCAST_PENDING;
        Serial.println("Status: Broadcasting UDP packet to network...");

        //add another 5 seconds wait time till next broadcast
        udpLastBroadcastTime = millis() + 5000;
        udpBroadcastAttempt++;
      }else{
        remoteServerIp.fromString(REMOTESERVER_IP);
        remoteServerPort = REMOTESERVER_PORT;
        currentStatus = STATUS_SERVER_NOTCONNECTED;

        //wait 5 seconds before we broadcast again
        udpBroadcastAttempt = 0;
        udpLastBroadcastTime = millis();
      }
  }

  //process incoming UDP packet when no remote server found yet
  if(currentStatus == STATUS_BCAST_PENDING){
    int packetSize = Udp.parsePacket();
    if (packetSize) {
      String msg = readUDPPacket(packetSize);
      String ports = msg.substring(msg.indexOf(";") + 1, msg.lastIndexOf(";"));
      ports = ports.substring(ports.indexOf(";")+1);
      String tcpPort = ports.substring(0, ports.indexOf(";"));
      String udpPort = ports.substring(ports.indexOf(";")+1);

      //check where did the response came from, set that as the remote server
      remoteServerIp = Udp.remoteIP();
      if(networkMode == NET_TCP){
        remoteServerPort = tcpPort.toInt();
      }else{
        remoteServerPort = udpPort.toInt();
      }
      

      currentStatus = STATUS_SERVER_NOTCONNECTED;
      Serial.println("Status: Found remote server. Waiting to connect to server...");

      udpBroadcastAttempt = 0;
      udpLastBroadcastTime = millis();
    }
    return 0;
  }

  // when status is not connected, UDP handshake done or when tcp client not longer alive
  if(currentStatus == STATUS_SERVER_NOTCONNECTED){
    if(networkMode == NET_TCP){
      if(connectTCPServer()){
        String msg = NETMSG_CLIENT_HANDSHAKE;
        msg.replace("%1", name);
        msg.replace("%2", loginUser);
        msg.replace("%3", loginPassword);
        sendTCPMessage(msg, true);
      }else{
        if(remoteServerIp.toString() == REMOTESERVER_IP){
          remoteServerPort = -1;
          currentStatus = STATUS_BCAST_PENDING;
        }
      }
    }else if(networkMode == NET_UDP){
      if(udpBroadcastAttempt == 0){
        String msg = NETMSG_CLIENT_HANDSHAKE;
        msg.replace("%1", name);
        msg.replace("%2", loginUser);
        msg.replace("%3", loginPassword);
        sendUDPPacket(msg.c_str());
        udpLastBroadcastTime = millis() + 10000;
        udpBroadcastAttempt = 1;
      }else if(udpLastBroadcastTime <= millis()){
        //fail to receive any handshake response
        Serial.println("UDP: Fail to get valid response from server.");
        udpBroadcastAttempt = 0;
        delay(5000);
      }
    }
  }


  String response;
  if(networkMode == NET_TCP){
    if(tcpClient.connected() && tcpClient.available()) {
      response = readTCPMessage();
    }
  }else if(networkMode == NET_UDP){
    int packetSize = Udp.parsePacket();
    if (packetSize) {
      response = readUDPPacket(packetSize);
    }
  }

  if(response == ""){
    if(currentStatus == STATUS_SERVER_CONNECTED && (millis() - lastPingTime >= PING_LOST_MAX_TIME)){
        currentStatus = STATUS_SERVER_NOTCONNECTED;
    }
    return 0;
  }

  if(currentStatus < STATUS_SERVER_CONNECTED){
    if(response.startsWith(String(NETMSG_SERVER_HANDSHAKE))){
      currentStatus = STATUS_SERVER_CONNECTED;
      lastPingTime = millis();
      Serial.println("Status: Connected to remote server!");

      if(isServerConnectedCallbackAvailable){
        onServerConnectedCallback();
      }
    }else{
      Serial.println("TCP: Fail to get valid response from server. Response:" + response +".");
      delay(5000);
    }
  }else{
    if(response == NETMSG_SERVER_PING){
      if(networkMode == NET_TCP){
        sendTCPMessage(NETMSG_CLIENT_PING, false);
      }else{
        sendUDPPacket(NETMSG_CLIENT_PING.c_str());
      }
      lastPingTime = millis();
    }else {
      if(isServerMessageCallbackAvailable){
        onServerMessageCallback(response);
      }
      return 1;
    }
  } 
  return 0;
}

bool MMNetwork::isWiFiConnected(){
  #ifdef ESP32
    return WiFi.status() == WL_NO_SHIELD;
  #else
    return WiFi.status() == WL_CONNECTED;
  #endif
}

// Connect to WiFi
// return true on success, false on failure
bool MMNetwork::connectToWiFi(){
  if (multi.run() != WL_CONNECTED) {
    Serial.println("WiFi: Unable to connect to network");
    return false;
  }

  Serial.print("WiFi: Connected with IP: ");
  Serial.println(WiFi.localIP());
  myLocalIp = WiFi.localIP();
  wifiMacAddress = WiFi.macAddress();

  String ip = WiFi.localIP().toString();
  if(UDP_BROADCAST_IP == ""){
    udpBroadcastIp = ip.substring(0, ip.lastIndexOf(".")+1) + "255";
  }else{
    udpBroadcastIp = UDP_BROADCAST_IP;
  }
  
  return true;
}

// Read UDP Pack
String MMNetwork::readUDPPacket(int packetSize){
  char udpPacketBuffer[UDP_TX_PACKET_MAX_SIZE + 1];
  // read the packet into packetBufffer
  int n = Udp.read(udpPacketBuffer, UDP_TX_PACKET_MAX_SIZE);
  udpPacketBuffer[n] = 0;
  return String(udpPacketBuffer);
}

// Send UDP Packet
void MMNetwork::sendBroadcastUDPPacket(const char* message){
  Udp.beginPacket(udpBroadcastIp.c_str(), UDP_BCAST_PORT);
  Udp.println(message);
  Udp.endPacket();
}

// Send UDP Packet
void MMNetwork::sendUDPPacket(const char* message){
  Udp.beginPacket(remoteServerIp.toString().c_str(), remoteServerPort);
  Udp.println(message);
  Udp.endPacket();
}

// Connect to TCP Server
bool MMNetwork::connectTCPServer(){
  if (!tcpClient.connect(remoteServerIp, remoteServerPort)) {
    Serial.println("TCP: Fail to establish connection with remote server...");
    tcpClient.stop();
    return false;
  }
  return true;
}

// Send TCP message
void MMNetwork::sendTCPMessage(String message, bool waitTimeout){
  // This will send a string to the server
  if (tcpClient.connected()) {
    Serial.println(message);
    tcpClient.println(message);
  }

  if(waitTimeout){
    unsigned long timeout = millis();
    while (tcpClient.available() == 0) {
      if (millis() - timeout > 5000) {
        Serial.println("TCP: Fail to receive anything from remote server...");
        break;
      }
    }
  }
}

// Read TCP message
String MMNetwork::readTCPMessage(){
  String result = "";

  //we are using \n to break message incase they fused into 1 packet by TCP/IP stack
  //TODO: Use a more reliable TCP message protocol
  while (tcpClient.available() && tcpClient.peek()) {
    char ch = static_cast<char>(tcpClient.read());
    if(ch == '\n'){
      return result;
    }
    result += ch;
  }

  return result;
}

void MMNetwork::setServerMessageCallback(cbfunc_t callbackFunction) {
    onServerMessageCallback = callbackFunction;
    isServerMessageCallbackAvailable = true;
}

void MMNetwork::setServerConnectedCallback(cbfunc_c callbackFunction) {
    onServerConnectedCallback = callbackFunction;
    isServerConnectedCallbackAvailable = true;
}

// Return current status
int MMNetwork::getNetworkStatus(){
  return currentStatus;
}

// Return if client is connected
bool MMNetwork::isServerConnected(){
  return tcpClient.connected();
}

// Send message
void MMNetwork::sendMessage(String message){
  // This will send a string to the server
  if (currentStatus == STATUS_SERVER_CONNECTED) {
    if(networkMode == NET_TCP){
      sendTCPMessage(message, false);
    }else{
      sendUDPPacket(message.c_str());
    }
  }
}

//set login username
void MMNetwork::setLoginUsername(String username){
  this->loginUser = username;
}

//set login password
void MMNetwork::setLoginPassword(String password){
  this->loginPassword = password;
}

//add wifi ssid
void MMNetwork::addWiFiSSID(String ssid, String password){
  multi.addAP(ssid.c_str(), password.c_str());
}

//set wifi mode
void MMNetwork::setWiFiMode(int mode){
  this->wifiMode = mode;
}

//set wifi ap ssid
void MMNetwork::setWiFiAPSSID(String ssid){
  this->apWifiSSID = ssid;
}

//set wifi ap password
void MMNetwork::setWiFiAPPassword(String password){
  this->apWifiPassword = password;
}