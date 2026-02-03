#include <net/ipv4.h>

using namespace os;
using namespace os::common;
using namespace os::net;


InternetProtocolHandler::InternetProtocolHandler(InternetProtocolProvider* backend, uint8_t protocol) {
  this->backend = backend;
  this->ip_protocol = protocol;
  backend->handlers[protocol] = this;
}


InternetProtocolHandler::~InternetProtocolHandler() {
  if (backend->handlers[ip_protocol] == this)
    backend->handlers[ip_protocol] = 0;
}


bool InternetProtocolHandler::OnInternetProtcolReceived(uint32_t srcIP_BE, uint32_t dstIP_BE, uint8_t* internetprotocolPayload, uint32_t size) {
  return false; // default case
}


void InternetProtocolHandler::Send(uint32_t dstIP_BE, uint8_t* internetprotocolPayload, uint32_t size) {
  backend->Send(dstIP_BE, ip_protocol, internetprotocolPayload, size);
}


InternetProtocolProvider::InternetProtocolProvider(EtherFrameProvider* backend, AddressResolutionProtocol* arp, uint32_t gatewayIP, uint32_t subnetMask) 
:EtherFrameHandler(backend, 0x0800) {
  // set all handlers to 0
  for (int i = 0; i < 255; i++)
    handlers[i] = 0;

  this->arp = arp;
  this->gatewayIP = gatewayIP;
  this->subnetMask = subnetMask;
}


InternetProtocolProvider::~InternetProtocolProvider() {
}


bool InternetProtocolProvider::OnEtherFrameReceived(uint8_t* etherframePayload, uint32_t size) {
}


void InternetProtocolProvider::Send(uint32_t dstIP_BE, uint8_t protocol, uint8_t* buffer, uint32_t size) {
}


uint16_t InternetProtocolProvider::Checksum(uint16_t* data, uint32_t lengthInBytes) {
}


