#ifndef __OS__DRIVERS__ATA_H
#define __OS__DRIVERS__ATA_H

#include <drivers/driver.h>
#include <common/types.h>
#include <hardwarecommunication/port.h>

namespace os {
  namespace drivers {

    class AdvancedTechnologyAttachment {
      protected:
        hardwarecommunication::Port16Bit dataPort;
        hardwarecommunication::Port16Bit errorPort;
        hardwarecommunication::Port16Bit sectorCountPort;
        hardwarecommunication::Port16Bit lbaLowPort;
        hardwarecommunication::Port16Bit lbaMidPort;
        hardwarecommunication::Port16Bit lbaHiPort;
        hardwarecommunication::Port16Bit devicePort;
        hardwarecommunication::Port16Bit commandPort;
        hardwarecommunication::Port16Bit controlPort;
        bool master;

        common::uint16_t bytesPerSector; // NOTE: hardcoded to 512 bytes

      public:
        AdvancedTechnologyAttachment(common::uint16_t portBase, bool master);
        ~AdvancedTechnologyAttachment();

        void Identify();
        void Read28(common::uint32_t sector, common::uint8_t* data, int count);
        void Write28(common::uint32_t sector, common::uint8_t* data, int count);
        void Flush();
    };

  }
}

#endif 
