#ifndef __DRIVER_H
#define __DRIVER_H

#include "types.h"

class Driver {
  public:
    Driver();
    ~Driver();

    // virtual functions allow for overriding by derived classes
    virtual void Activate();
    virtual int Reset(); // reset all the drivers so we have a controlled starting state each time
    virtual void Deactivate();
};

class DriverManager {
  private:
    // FIXME: change this when dynamic memory management is implemented
    Driver* drivers[255]; // Array of all the drivers, up to 255
    int numDrivers;

  public:
    DriverManager(); 
    void AddDriver(Driver*);
    void ActivateAll(); // activates all the drivers

};

#endif 
