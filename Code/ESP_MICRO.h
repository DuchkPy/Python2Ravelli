/* ESP8266 TO PY: THE MICRO LIBRARY
 * Originally written by Junicchi, Serial.print "debug" removed by DuchkPy
 * https://github.com/Kebablord

 * MAP
 - start(ssid,password)---> Connects to wifi with given username and password
 - waitUntilNewReq() -----> Waits until a new python request come, checks for requests regularly
 - returnThisStr(data) ---> sends your String data to localhost (python)
 - getPath() -------------> gets the request path as string, ex: https://192.113.133/ledON -> "ledON"
*/

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>

// PORT
WiFiServer server(80);
WiFiClient client;
String rule;

void start(String ssid, String pass){
	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid.c_str(),pass.c_str());

	// Wait for connection
	while(WiFi.status() != WL_CONNECTED) {
	delay(500);
	}
	// Setting up mDNS responder
	if(!MDNS.begin("esp8266")) {
		while(1) delay(1000);
	}
	// Start TCP (HTTP) server
	server.begin();
	//Serial.println("TCP server started");
	MDNS.addService("http", "tcp", 80);
}

bool isReqCame = false;

void CheckNewReq(){
	client = server.available();
	if(!client) return;
	// Waiting client to connect
	while(client.connected() && !client.available()) delay(1);
	// Read the first line of HTTP request
	String req = client.readStringUntil('\r');
	int addr_start = req.indexOf(' ');
	int addr_end = req.indexOf(' ', addr_start + 1);
	if(addr_start == -1 || addr_end == -1) return;
	
	req = req.substring(addr_start + 2, addr_end);
	
	rule = req;
	isReqCame = true;
	client.flush();
}
void waitUntilNewReq(){
	do{CheckNewReq();} while (!isReqCame);
	isReqCame = false;
}

void returnThisStr(String final_data){
	String s;
	//HTTP Protocol code.
	s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
	s += final_data; //Our final raw data to return
	client.print(s);
}

String getPath(){
	return rule;
}
