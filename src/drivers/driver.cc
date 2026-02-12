#include <drivers/driver.h>

using namespace os::common;
using namespace os::drivers;


Driver::Driver(){

};

Driver::~Driver(){

};

void Driver::Activate() {

};


int Driver::Reset() {
  return 0;
};

void Driver::Deactivate() {

};

DriverManager::DriverManager() {
  numDrivers = 0;
};

void DriverManager::AddDriver(Driver* drv) {
  drivers[numDrivers] = drv;
  numDrivers++;
};

void DriverManager::ActivateAll() {
  for (uint8_t i = 0; i < numDrivers; i++) {
    drivers[i]->Activate();
  }
};
