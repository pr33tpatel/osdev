#ifndef __OS__GUI__WIDGET_H
#define __OS__GUI__WIDGET_H

#include <common/types.h>
#include <drivers/keyboard.h>
#include <common/graphicscontext.h>

namespace os {
  namespace gui {

    class Widget : public os::drivers::KeyboardEventHandler {

      protected:
        Widget* parent;
        common::int32_t x;
        common::int32_t y;
        common::int32_t w;
        common::int32_t h;

        common::uint8_t r;
        common::uint8_t g;
        common::uint8_t b;
        
        bool Foucssable;

      public:

        Widget( Widget* parent,
                common::int32_t x, common::int32_t y, common::int32_t w, common::int32_t h,
                common::uint8_t r, common::uint8_t g, common::uint8_t b );
        ~Widget();

        virtual void GetFocus(Widget* widget);
        virtual void ModelToScreen(common::uint32_t &x, common::uint32_t &y);
        virtual bool ContainsCoordinate(common::uint32_t x, common::uint32_t y);
        
        virtual void Draw(common::GraphicsContext* gc);
        virtual void OnMouseDown(common::uint32_t x, common::uint32_t y, common::uint8_t button);
        virtual void OnMouseUp(common::uint32_t x, common::uint32_t y, common::uint8_t button);
        virtual void OnMouseMove(common::uint32_t oldx, common::uint32_t oldy,  common::uint32_t newx, common::uint32_t newy);
    };



    class CompositeWidget : public Widget {
      private:
        Widget* children[100]; // NOTE: array of pointers to the children
        int numChildren;
        Widget* focusedChild;

      public:
        
        CompositeWidget( Widget* parent,
                common::int32_t x, common::int32_t y, common::int32_t w, common::int32_t h,
                common::uint8_t r, common::uint8_t g, common::uint8_t b );

        ~CompositeWidget();
      
        virtual void GetFocus(Widget *widget);
        // virtual void ModelToScreen(common::uint32_t &x, common::uint32_t &y);
        virtual bool AddChild(Widget *child);
        
        virtual void Draw(common::GraphicsContext* gc);
        virtual void OnMouseDown(common::uint32_t x, common::uint32_t y, common::uint8_t button);
        virtual void OnMouseUp(common::uint32_t x, common::uint32_t y, common::uint8_t button);
        virtual void OnMouseMove(common::uint32_t oldx, common::uint32_t oldy,  common::uint32_t newx, common::uint32_t newy);

        virtual void OnKeyDown(char str);
        virtual void OnKeyUp(char str);
        
    };
  }
}

#endif
