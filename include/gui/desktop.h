#ifndef __OS__GUI__DESKTOP_H
#define __OS__GUI__DESKTOP_H

#include <common/types.h>
#include <drivers/mouse.h>
#include <gui/widget.h>

namespace os {
  namespace gui {

    // NOTE: the Desktop is a CompositeWidget because it can contain (is composed of) multiple widgets on it
    class Desktop : public CompositeWidget, public os::drivers::MouseEventHandler {

      protected:
        // NOTE: Mouse{X,Y} are raw mouse coordinates
        os::common::int32_t MouseX; 
        os::common::int32_t MouseY;

        // FIXME: this could be a horrible design, why is the MouseSensitivity a part of the Desktop and not the mouse, i dont know ?
        float MouseSensitivity = 0.50f; // NOTE: used to control the speed of the mouse, higher value means faster

      public:
        Desktop(
            os::common::int32_t w, os::common::int32_t h,
            os::common::uint32_t r, os::common::uint32_t g, os::common::uint32_t b);
        ~Desktop();

        void Draw(common::GraphicsContext* gc); 
        void OnMouseDown(os::common::uint8_t button);
        void OnMouseUp(os::common::uint8_t button);
        void OnMouseMove(int x, int y);
    };

  }
}

#endif
