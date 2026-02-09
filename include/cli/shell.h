#ifndef __OS__CLI__SHELL_H
#define __OS__CLI__SHELL_H

#include <common/types.h>
#include <drivers/keyboard.h>
#include <hardwarecommunication/pci.h>
#include <utils/string.h>
#include <utils/print.h>

namespace os {
  namespace cli {

    class Shell : public os::drivers::KeyboardEventHandler{
      private:
        char commandbuffer[256];  // [stores characters in command buffer]
        common::uint16_t bufferIndex; // [indexer for the command buffer]

        // reference to PCI to run 'lspci'
        os::hardwarecommunication::PeripheralComponentInterconnectController* pci;

      public:
        Shell();
        ~Shell();

        // event handler override
        void OnKeyDown(char c) override;

        /**
         * @brief [fills command buffer with specifed char to specifed length ]
         * @param char [specifed char to fill with, default is ' ']
         * @param length [specified length to clear, default is entire buffer]
         */
        void fillCommandBuffer(char fill_char = ' ', uint16_t length = sizeof(commandbuffer)/sizeof(commandbuffer[0]));

        // [link the pci driver to the shell]
        void SetPCI(os::hardwarecommunication::PeripheralComponentInterconnectController* pciController);  

        void PrintPrompt();

      private:
        void ExecuteCommand();

    };
  }
}

#endif
