#include <net/arp.h>

using namespace os;
using namespace common;
using namespace net;

void printf(const char* str);
void printfHex(uint8_t key );
void printfHex8Bytes(uint8_t key );


AddressResolutionProtocol::AddressResolutionProtocol(EtherFrameProvider* backend)
:  EtherFrameHandler(backend, 0x806)
{
    numCacheEntries = 0;
}

AddressResolutionProtocol::~AddressResolutionProtocol()
{
}

void AddressResolutionProtocol::printfIPAddress(common::uint32_t IP) {
  /* little endian x86 machine reading big endian packet */
  printfHex((IP)       & 0xFF); printf(".");
  printfHex((IP >>  8) & 0xFF); printf(".");
  printfHex((IP >> 16) & 0xFF); printf(".");
  printfHex((IP >> 24) & 0xFF); 

}
void AddressResolutionProtocol::printfMACAddress(common::uint64_t MAC) {
  /* little endian x86 machine reading big endian */
  printfHex((MAC      ) & 0xFF); printf(".");
  printfHex((MAC >>  8) & 0xFF); printf(".");
  printfHex((MAC >> 16) & 0xFF); printf(".");
  printfHex((MAC >> 24) & 0xFF); printf(".");
  printfHex((MAC >> 32) & 0xFF); printf(".");
  printfHex((MAC >> 40) & 0xFF); 


}

void AddressResolutionProtocol::printfARPmsg(AddressResolutionProtocolMessage* arp) {
  printf("\nARP PACKET:\n");

  // opcode
  printf("Opcode: ");
  if (arp->command == 0x0100)
    printf("REQUEST");
  else if (arp->command == 0x0200) 
    printf("REPLY");
  else printfHex(arp->command);

  // sender MAC 
  printf("\nSource MAC:        ");
  printfMACAddress(arp->srcMAC);

  // sender IP
  printf("        Source IP:         ");
  printfIPAddress(arp->srcIP);

  // destination MAC
  printf("\nDestination MAC:   ");
  printfMACAddress(arp->dstMAC);

  // destination IP
  printf("        Destination IP:    ");
  printfIPAddress(arp->dstIP);

  printf("\nARP PACKET END.\n");
}
            
bool AddressResolutionProtocol::OnEtherFrameReceived(uint8_t* etherframePayload, uint32_t size)
{
    if(size < sizeof(AddressResolutionProtocolMessage))
        return false;
    
    AddressResolutionProtocolMessage* arp = (AddressResolutionProtocolMessage*)etherframePayload;
    if(arp->hardwareType == 0x0100)
    {
        
        if(arp->protocol == 0x0008
        && arp->hardwareAddressSize == 6
        && arp->protocolAddressSize == 4
        && arp->dstIP == backend->GetIPAddress())
        {
            
            printfARPmsg(arp);
            switch(arp->command)
            {
                
                case 0x0100: // request
                    arp->command = 0x0200;
                    arp->dstIP = arp->srcIP;
                    arp->dstMAC = arp->srcMAC;
                    arp->srcIP = backend->GetIPAddress();
                    arp->srcMAC = backend->GetMACAddress();
                    return true;
                    break;
                    
                case 0x0200: // response
                    if(numCacheEntries < 128)
                    {
                        IPcache[numCacheEntries] = arp->srcIP;
                        MACcache[numCacheEntries] = arp->srcMAC;
                        numCacheEntries++;
                    }
                    break;
            }
        }
        
    }
    
    return false;
}

void AddressResolutionProtocol::RequestMACAddress(uint32_t IP_BE)
{

  AddressResolutionProtocolMessage arp;
  arp.hardwareType = 0x0100; // ethernet
  arp.protocol = 0x0008; // ipv4
  arp.hardwareAddressSize = 6; // mac
  arp.protocolAddressSize = 4; // ipv4
  arp.command = 0x0100; // request

  arp.srcMAC = backend->GetMACAddress();
  arp.srcIP = backend->GetIPAddress();
  arp.dstMAC = 0xFFFFFFFFFFFF; // broadcast
  arp.dstIP = IP_BE;

  this->Send(arp.dstMAC, (uint8_t*)&arp, sizeof(AddressResolutionProtocolMessage));

}

uint64_t AddressResolutionProtocol::GetMACFromCache(uint32_t IP_BE)
{
    for(int i = 0; i < numCacheEntries; i++)
        if(IPcache[i] == IP_BE)
            return MACcache[i];
    return 0xFFFFFFFFFFFF; // broadcast address
}

uint64_t AddressResolutionProtocol::Resolve(common::uint32_t IP_BE) {


	uint64_t result = GetMACFromCache(IP_BE);

	if (result == 0xffffffffffff) {
	
		RequestMACAddress(IP_BE);
	}

	
	uint8_t attempts = 0;

	while (result == 0xffffffffffff && attempts < 128) {
	
		result = this->GetMACFromCache(IP_BE);
		attempts++;

	}

	if (attempts >= 128) {
	
		printf("\nARP Resolve Time Out.\n");
	}
	
	return result;
}
