// EtherDune DNS Service
// Author: Javier Peletier <jm@friendev.com>
// Summary: Implements the basics of DNS so as to resolve hostnames to IP addresses
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

#include "DNS.h"
#include "Checksum.h"

#define AC_LOGLEVEL 3
#include <ACLog.h>
ACROSS_MODULE("DNS");

void DNSClient::onReceive(uint16_t len)
{
	//Cheap hack: usually the last 4 bytes of the DNS response are the IP address we're looking for.

	IPAddress& resolvedIP = *((IPAddress*)(packet.udpData + len - sizeof(IPAddress)));

	NetworkService::notifyOnDNSResolve(packet.dns.rcode, packet.dns.identification, resolvedIP);
	buffer.release();
	if (!buffer.isEmpty())
		send();
}

DNSClient::DNSClient() 
{
	remotePort = 53;
}

/// <summary>
/// Resolves a host name to an IP address
/// </summary>
/// <param name="name">host name to resolve</param>
/// <returns>Returns an numeric id token to match against the identification parameter of \ref onDNSResolve virtual callback function </returns>
uint16_t DNSClient::resolve(const char* name)
{
	remoteIP = dnsIP;

	timer = DNS_TIMEOUT_QUERY;

	DNSHeader& header = *(DNSHeader*)packet.raw;

	uint8_t* queryPtr = packet.raw + sizeof(DNSHeader);
	uint8_t* b = queryPtr;
	uint8_t* label = b;
	b++;

	for (*label = 0; *name != 0; name++, b++)
	{
		if (*name == '.')
		{
			label = b;

			*label = 0;
		}
		else
		{
			*b = *name;
			(*label)++;
		}
	}

	*b = 0;

	uint16_t id = Checksum::calc(b - queryPtr, queryPtr);

	header.zero();
	header.identification = id;
	/*
	header.QR = 0; // query
	header.opcode = 0; // standard query;
	*/
	header.RD = 1; //recursion desired
	header.numberOfQuestions=1; //only one question

	//queryType = 1 (A query), queryClass=1 (Internet Address);
	b++;
	*((uint32_t*)b) = 0x01000100; //queryType = 1 (A query), queryClass=1 (Internet Address);


	b += 4;

	write(sizeof(header) + b - queryPtr, packet.raw);

	send();
	return id;
}


void DNSClient::tick()
{
	if (timer > 0)
	{
		timer--;

		if (timer == 0)
		{
			buffer.release();
			timer = DNS_TIMEOUT_QUERY;
		}
	}


	UDPSocket::tick();
}

bool DNSClient::sendPacket()
{
	uint16_t dataChecksum = 0;
	uint16_t dataLength;

	dataLength = buffer.fillTxBuffer(sizeof(EthernetHeader) + sizeof(IPHeader) + sizeof(UDPHeader),/*out*/ dataChecksum, 1);

	bool ret;
	if (dataLength > 0)
	{
		prepareUDPPacket(dataLength, dataChecksum);
		ret = !sendIPPacket(sizeof(IPHeader) + sizeof(UDPHeader));
	}
	else
		ret = false;
	
	return ret;

}