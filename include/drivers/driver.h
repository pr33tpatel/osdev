#ifndef __OS__DRIVERS__DRIVER_H
#define __OS__DRIVERS__DRIVER_H

#include <common/types.h>

namespace os {
namespace drivers {


class Driver {
 public:
  Driver();
  ~Driver();

  // virtual functions allow for overriding by derived classes
  virtual void Activate();
  virtual int Reset();  // reset all the drivers so we have a controlled starting state each time
  virtual void Deactivate();
};

class DriverManager {
  // private:
 public:  // FIXME: change back to private after testing
  // FIXME: change this when dynamic memory management is implemented
  Driver* drivers[255];  // Array of all the drivers, up to 255
  int numDrivers;

 public:
  DriverManager();
  void AddDriver(Driver*);
  void ActivateAll();  // activates all the drivers
};

}  // namespace drivers
}  // namespace os
#endif
