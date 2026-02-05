#ifndef __OS__NET_ARP_H
#define __OS__NET_ARP_H

#include <common/types.h>
#include <net/etherframe.h>
#include <utils/print.h>

namespace os {
  namespace net {

    struct AddressResolutionProtocolMessage {
      common::uint16_t hardwareType;
      common::uint16_t protocol;
      common::uint8_t hardwareAddressSize; // NOTE: hardcoded 6, MAC address is 48 bits = 6 bytes
      common::uint8_t protocolAddressSize; // NOTE: hardcoded 4, ipv4 address is 32 bits = 4 bytes
      common::uint16_t command;

      common::uint64_t srcMAC : 48;
      common::uint32_t srcIP;
      common::uint64_t dstMAC : 48;
      common::uint32_t dstIP;
    
    } __attribute__((packed));
    
    class AddressResolutionProtocol : public EtherFrameHandler {

      private:
        // FIXME: implement persistent storage of cache in hard drive
        common::uint32_t IPcache[128];
        common::uint64_t MACcache[128];
        int numCacheEntries;

      public:
        AddressResolutionProtocol(EtherFrameProvider* backend);
        ~AddressResolutionProtocol();

        void printIPAddress(common::uint32_t IP);
        void printMACAddress(common::uint64_t MAC);
        void printARPmsg(AddressResolutionProtocolMessage* arp);
        bool OnEtherFrameReceived(common::uint8_t* etherframePayload, common::uint32_t size);

        void RequestMACAddress(common::uint32_t IP_BE);
        common::uint64_t GetMACFromCache(common::uint32_t IP_BE);
        common::uint64_t Resolve(common::uint32_t IP_BE);

    };

    
  }
}

#endif
