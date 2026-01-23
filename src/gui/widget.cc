#include <cwchar>
#include <gui/widget.h>

using namespace os::common;
using namespace os::gui;



Widget::Widget( Widget* parent,
                int32_t x, int32_t y, int32_t w, int32_t h,
                uint8_t r, uint8_t g, uint8_t b ) 
: KeyboardEventHandler()
{

  this->parent = parent;
  this->x = x;
  this->y = y;
  this->w = w;
  this->h = h;
  this->r = r;
  this->g = g;
  this->b = b;
  this->Foucssable = true;
  
  /* NOTE:
   * this-x := x position of widget
   * this-y := x position of widget
   * this-w := width of widget
   * this-h := height of widget
   */
}

Widget::~Widget() {
}


void Widget::GetFocus(Widget* widget) {
  if(parent != 0) {
    parent->GetFocus(widget); // recursive
  }
}


void Widget::ModelToScreen(uint32_t &x, uint32_t &y) {
  if(parent != 0) 
    parent->ModelToScreen(x, y); 

  // to get the absolute coordinates, get the coordinates from the parent + this windows coordinates
  x += this->x;
  y += this->y;
}


void Widget::Draw(GraphicsContext* gc) {
  // X and Y are absolute coordinates
  uint32_t X = 0;
  uint32_t Y = 0;
  ModelToScreen(X,Y); 
  gc->FillRectangle(X,Y,w,h, r,g,b);
}


bool Widget::ContainsCoordinate(uint32_t x, uint32_t y) {
  return 
    // NOTE: a full screen widget would start at (0,0), so full screen widget would always return `true` to ContainsCoordinate
            this->x <= x  // relative x is less than or equal to absolute x (x of coordinate is less than x of widget)
         && x < this->x + this->w // relative x is less than absolute x + width of widget (the x position of widget + width of widget)
         && this->y <= y  // relative y is less than or equal to absolute y
         && y < this->y + this->h; // relative y is less than absolute y + height of widget
}

void Widget::OnMouseDown(uint32_t x, uint32_t y, uint8_t button) {
  if(Foucssable)
    GetFocus(this);
}


void Widget::OnMouseUp(uint32_t x, uint32_t y, uint8_t button) {

}


void Widget::OnMouseMove(uint32_t oldx, uint32_t oldy,  uint32_t newx, uint32_t newy) {
}




/* Composite Widget Class*/
CompositeWidget::CompositeWidget(Widget* parent,
                                  int32_t x, int32_t y, int32_t w, int32_t h,
                                  uint8_t r, uint8_t g, uint8_t b )
: Widget(parent, x,y,w,h, r,g,b) 
{
  focusedChild = 0;
  numChildren = 0;
}


CompositeWidget::~CompositeWidget() {
}


void CompositeWidget::GetFocus(Widget* widget){
  this->focusedChild = widget;
  if(parent != 0) 
    parent->GetFocus(this);
}


bool CompositeWidget::AddChild(Widget *child) {
  if(numChildren >= 100)  // INFO: max children is 100, defined in widget.h
    return false;

  children[numChildren++] = child; // add the pointer of the child to the array of pointers 
  return true; 
}


void CompositeWidget::Draw(GraphicsContext* gc) {
  Widget::Draw(gc); // draws the background
  for (int i = numChildren-1; i >= 0; --i) {
    children[i]->Draw(gc);
  }
}


void CompositeWidget::OnMouseDown(uint32_t x, uint32_t y, uint8_t button) {
  for(int i = 0; i < numChildren; i++) 
    if(children[i]->ContainsCoordinate(x - this->x, y - this->y)) {
      children[i]->OnMouseDown(x - this->x, y - this->y, button);
      break;
    }
}


void CompositeWidget::OnMouseUp(uint32_t x, uint32_t y, uint8_t button) {
  for(int i = 0; i < numChildren; i++) 
    if(children[i]->ContainsCoordinate(x - this->x, y - this->y)) {
      children[i]->OnMouseUp(x - this->x, y - this->y, button);
      break;
    }
}


void CompositeWidget::OnMouseMove(uint32_t oldx, uint32_t oldy,  uint32_t newx, uint32_t newy) {
  /* FIXME: OnMouseMove code is really ugly. instead there should be functions like OnMouseEnter and OnMouseLeave which are called whenever the mouse is travelling between widgets */
  int firstChild = -1;
  for(int i = 0; i < numChildren; i++) 
    if(children[i]->ContainsCoordinate(oldx - this->x, oldy - this->y)) {
      children[i]->OnMouseMove(oldx - this->x, oldy - this->y, newx - this->x, newy - this->y);
      firstChild = i;
      break;
    }

  for(int i = 0; i < numChildren; i++) 
    if(children[i]->ContainsCoordinate(newx - this->x, newy - this->y)) {
      if(firstChild != i) 
        children[i]->OnMouseMove(oldx - this->x, oldy - this->y, newx - this->x, newy - this->y);
      break;
    }
}


void CompositeWidget::OnKeyDown(char str) {
  if(focusedChild != 0)
    focusedChild->OnKeyDown(str);
}


void CompositeWidget::OnKeyUp(char str) {
  if(focusedChild != 0)
    focusedChild->OnKeyUp(str);
}


