/*
https://datasheets.maximintegrated.com/en/ds/DS18B20.pdf	
*/

#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include <WiFiClient.h>
#include <ESP8266WebServer.h>

// pin 20 = gpio 2 (one wire bus / DS18D20 sensor)
const byte PIN_20 = 2;

const char* AP_SSID = "your wifi ap";
const char* AP_PASSWORD = "your ap password";

OneWire oneWire(PIN_20);
DallasTemperature DS18B20(&oneWire);
ESP8266WebServer server(80);

// functions prototype
void web_server_init();
void wifi_connect();
float get_temp();
void handle_index();

void setup() {
	Serial.begin(115200);

	wifi_connect();
	web_server_init();
}

void loop() {
// check for incomming client connections frequently in the main loop:
	server.handleClient();
}

void web_server_init() {	
	server.on("/", handle_index()); 
	
/*	
	server.on("/temp", []() {
		float temp = get_temp(); // live sensor reader
		String to_send = "Temperature: " + String((int) temp) + "° C";
		server.send(200, "text/plain", to_send);
	});
*/
	server.on("/temp", handle_temp());
	server.on("/status", handle_status());
	
	server.begin();
	Serial.println("HTTP server started");	
}

void wifi_connect() {
	Serial.println("Connecting to AP " + (String) AP_SSID);
	WiFi.begin(AP_SSID, AP_PASSWORD);
	while (WiFi.status() != WL_CONNECTED) {
		delay(1000);
		Serial.print(".");
	}

	Serial.println("\nWiFi connected to " + (String) AP_SSID);
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());
}

float get_temp() {
	DS18B20.requestTemperatures();
	float temp = DS18B20.getTempCByIndex(0);
	
	Serial.print("Temperature: ");
	Serial.println(temp);
//	while (temp == 85.0 || temp == (-127.0));

	return temp;
}

void handle_index() {
//	server.send(200, "text/plain", "This is an index page.");
	server.send(200, "text/plain", "Hello from ESP8266, read from /temp or /status");
}

void handle_temp() {
	float temp = get_temp(); // on demand sensor reader
	String to_send = "Temperature: " + String((int) temp) + " C";
	server.send(200, "text/plain", to_send);
}

void handle_status() {
	String to_send = "just a test status page :)";
	server.send(200, "text/plain", to_send);
}
