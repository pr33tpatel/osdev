#ifndef __OS__CIU__OFFICER_H
#define __OS__CIU__OFFICER_H

#include <ciu/report.h>
#include <utils/print.h>

namespace os {
namespace ciu {
class CIUOfficer {
 private:
  const char* subsystem;  // stores the identity of this officer
 public:
  explicit CIUOfficer(const char* subsystemName);

  void send(CIUReport& report);

  void trace(const char* code, const char* message);
  void info(const char* code, const char* message);
  void warning(const char* code, const char* message);
  void error(const char* code, const char* message);
  void critical(const char* code, const char* message);
};
}  // namespace ciu
}  // namespace os

#endif
