#include "gui/widget.h"
#include <gui/desktop.h>

using namespace os;
using namespace os::common;
using namespace os::gui;

Desktop::Desktop(
    int32_t w, int32_t h,
    uint32_t r, uint32_t g, uint32_t b)
  // NOTE: parent is 0 because the Desktop is the `root` widget
: CompositeWidget(0, 0, 0, w, h, r, g, b),
  MouseEventHandler()
{
  // center the mouse
  MouseX = w/2;
  MouseY = h/2;

  float MouseSensitivity = 0.25f; // NOTE: used to control the speed of the mouse, higher value means faster
}


Desktop::~Desktop() {
}


void Desktop::Draw(GraphicsContext *gc) {
  // draw background ( i think ? )
  CompositeWidget::Draw(gc);

  for (int i = 0; i < 4; i++) {
    // NOTE: Draw the cursor, each PutPixel is a "stem" of the cursor flower 
    gc->PutPixel(MouseX-i, MouseY, 0x00, 0x00, 0x00);  
    gc->PutPixel(MouseX+i, MouseY, 0x00, 0x00, 0x00);
    gc->PutPixel(MouseX, MouseY-i, 0x00, 0x00, 0x00);
    gc->PutPixel(MouseX, MouseY+i, 0x00, 0x00, 0x00);
  }
}


void Desktop::OnMouseDown(uint8_t button) {
  CompositeWidget::OnMouseDown(MouseX, MouseY, button);
}


void Desktop::OnMouseUp(uint8_t button) {
  CompositeWidget::OnMouseUp(MouseX, MouseY, button);
}


void Desktop::OnMouseMove(int x, int y) {

  // x = x / (1/MouseSensitivity); // NOTE: divide by MouseSensitivity^-1 (ex: if MS = 0.25 => x = x / (1/0.25) => x / 4)
  // y = y / (1/MouseSensitivity);
  x /= 4;
  y /= 4;


  // bounds checking for x coordinate
  int newMouseX = MouseX + x;
  if(newMouseX < 0)   newMouseX = 0;      // bounds check: left border of screen
  if(newMouseX >= w)  newMouseX = w - 1;  // bounds check: right border of screen
  
  // bounds checking for y coordinate
  int newMouseY = MouseY + y;
  if(newMouseY < 0)   newMouseY = 0;      // bounds check: floor (bottom) of screen
  if(newMouseY >= h)  newMouseY = h - 1;  // bounds check: ceiling (top) of screen


  CompositeWidget::OnMouseMove(MouseX, MouseY, newMouseX, newMouseY);

  MouseX = newMouseX;
  MouseY = newMouseY;
}

