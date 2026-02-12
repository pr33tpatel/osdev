#include <net/icmp.h>

using namespace os;
using namespace os::common;
using namespace os::net;
using namespace os::utils;

InternetControlMessageProtocol::InternetControlMessageProtocol(InternetProtocolProvider* backend)
    : InternetProtocolHandler(backend, 0x01) {
}


InternetControlMessageProtocol::~InternetControlMessageProtocol() {
}


bool InternetControlMessageProtocol::OnInternetProtocolReceived(
    uint32_t srcIP_BE, uint32_t dstIP_BE, uint8_t* internetprotocolPayload, uint32_t size
) {
  printf("ICMP RECV\n");
  if (size < sizeof(InternetControlMessageProtocolMessage)) return false;

  InternetControlMessageProtocolMessage* msg = (InternetControlMessageProtocolMessage*)internetprotocolPayload;

  switch (msg->type) {
    case 0:  // answer to ping
      printf("ping response from: 0x%08x\n", srcIP_BE);
      return false;

    case 8:
      msg->type = 0;  // response
      msg->checksum = 0;
      msg->checksum = InternetProtocolProvider::Checksum((uint16_t*)msg, sizeof(InternetControlMessageProtocolMessage));
      return true;
  }

  return false;
}


void InternetControlMessageProtocol::Ping(uint32_t ip_BE) {
  InternetControlMessageProtocolMessage icmp;
  icmp.type = 8;  // type 8 is we are being pinged
  icmp.code = 0;
  icmp.data = 0x3713;  // "leet" in BE
  icmp.checksum = 0;
  icmp.checksum = InternetProtocolProvider::Checksum((uint16_t*)&icmp, sizeof(InternetControlMessageProtocolMessage));

  InternetProtocolHandler::Send(ip_BE, (uint8_t*)&icmp, sizeof(InternetControlMessageProtocolMessage));
}
