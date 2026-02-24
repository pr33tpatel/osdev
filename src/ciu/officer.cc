#include <ciu/ciu.h>
#include <ciu/officer.h>


using namespace os;
using namespace os::ciu;
using namespace os::utils;


CIUOfficer::CIUOfficer(const char* subsystemName) : subsystem(subsystemName) {}


void CIUOfficer::send(CIUReport& report) {
  CIU::Report(report);
}


void CIUOfficer::trace(const char* code, const char* message) {
  CIUReport report(CIUSeverity::Trace, subsystem, code, message);
  send(report);
}


void CIUOfficer::info(const char* code, const char* message) {
  CIUReport report(CIUSeverity::Info, subsystem, code, message);
  send(report);
}


void CIUOfficer::warning(const char* code, const char* message) {
  CIUReport report(CIUSeverity::Warning, subsystem, code, message);
  send(report);
}


void CIUOfficer::error(const char* code, const char* message) {
  CIUReport report(CIUSeverity::Error, subsystem, code, message);
  send(report);
}


void CIUOfficer::critical(const char* code, const char* message) {
  CIUReport report(CIUSeverity::Critical, subsystem, code, message);
  send(report);
}
