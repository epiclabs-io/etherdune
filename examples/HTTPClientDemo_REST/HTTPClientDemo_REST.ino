// EtherDune HTTPClient class demo
// Author: Javier Peletier <jm@friendev.com>
// Summary: Demonstrates the HTTPClient class by connecting to a weather REST service
//
// Copyright (c) 2015 All Rights Reserved, http://friendev.com
//
// This source is subject to the GPLv2 license.
// Please see the License.txt file for more information.
// All other rights reserved.
//
// THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY 
// KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.

/// \file
/// \details Demonstrates the HTTPClient class by connecting to a weather REST service and 
/// extracting the local temperature of a city.

/// See the HTTPClient documentation for more information.
/// \cond

#include <ACross.h>
#include <FlowScanner.h>
#include <HTTPClient.h>


#define AC_LOGLEVEL 6
#include <ACLog.h>
ACROSS_MODULE("HTTPClient demo");

#define DHCP_ENABLE true

static const uint8_t CS_PIN = 10; //Put here what pin you are using for your ENC28J60's chip select
static MACAddress_P mymac = { 0x66, 0x72, 0x69, 0x65, 0x6e, 0x64 };

//the following parameters will be ignored and not linked if DHCP_ENABLE is set to true.
static IPAddress_P gatewayIP = { 192, 168, 1, 1 };
static IPAddress_P myIP = { 192, 168, 1, 33 };
static IPAddress_P netmask = { 255, 255, 255, 0 };
static IPAddress_P dns = { 8, 8, 8, 8 };


#if DHCP_ENABLE

#include <DHCP.h>
DHCP dhcp;

#else

#endif


DEFINE_FLOWPATTERN(temperaturePattern, "\"temp\":%7[^,]"); // look for a string like this "temp":12.321, and extract the values

class MyHTTPClient : public HTTPClient
{

public:

	char temp[8]; //temporary buffer to hold the captured temperature string

	void start()
	{
		request(F("api.openweathermap.org"), F("/data/2.5/weather?q=Madrid,es&units=metric"));
		//see http://openweathermap.org/current
	}

	void onHeaderReceived(uint16_t len, const byte* data)
	{
		//Serial.write(data, len);
	}

	void onResponseReceived()
	{
		ACTRACE("HTTP status=%d", statusCode);
		if (statusCode != HTTP_RESPONSE_OK)
		{
			Serial.print(F("Server error, status code="));
			Serial.println(statusCode);
			terminate();
		}

	}

	void onBodyBegin()
	{
		scanner.setPattern(temperaturePattern);
	}

	void onBodyReceived(uint16_t len, const byte* data)
	{
		ACTRACE("HTTP bytes received=%d", len);

		byte* buf = (byte*)data;
		
		if (temperaturePattern.signaled)
			return;

		if (scanner.scan(&buf, &len, temp))
		{
			float t = atof(temp);

			Serial.print(F("The temperature is "));
			Serial.print(t);
			Serial.println('C');
			close();
			return;
		}

	}
	void onResponseEnd()
	{
		ACTRACE("HTTP session end");
	}

}http;


unsigned long waitTimer = 0;

void setup()
{
	Serial.begin(115200);
	ACross::init();
	Serial.println(F("EtherDune HTTPClient sample"));
	Serial.print(F("Free RAM: ")); Serial.println(ACross::getFreeRam());

	Serial.println(F("Press any key to start..."));

	while (!Serial.available());

	net::localMAC = mymac;

	if (!net::begin(CS_PIN))
	{
		ACERROR("failed to start EtherDune");
		Serial.println(F("failed to start EtherDune"));
	}

	ACINFO("waiting for link...");

	while (!net::isLinkUp());

	ACINFO("link is up");


#if DHCP_ENABLE
	Serial.println("Obtaining DHCP configuration...");

	if (!dhcp.dhcpSetup())
	{
		Serial.println(F("DHCP setup failed"));
		ACross::halt(1);
	}

	Serial.println(F("DHCP setup OK"));

	Serial.println(F("DHCP config:"));
	Serial.print(F("Local IP: "));
	Serial.println(net::localIP.toString());
	Serial.print(F("Network mask: "));
	Serial.println(net::netmask.toString());
	Serial.print(F("Gateway IP: "));
	Serial.println(net::gatewayIP.toString());
	Serial.print(F("DNS IP: "));
	Serial.println(net::dnsIP.toString());
#else
	net::localIP = myIP;
	net::gatewayIP = gatewayIP;
	net::netmask = netmask;
	net::dnsIP = dns;

#endif

	http.start();

	Serial.println("Connecting...");

}



void loop()
{
	net::loop();

}

/// \endcond