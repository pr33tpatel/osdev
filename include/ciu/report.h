#ifndef __OS__CIU__REPORT_H
#define __OS__CIU__REPORT_H

#include <common/types.h>
#include <utils/ds/hashmap.h>
#include <utils/ds/linkedlist.h>
#include <utils/ds/pair.h>

namespace os {
namespace ciu {

enum class CIUSeverity { Trace, Info, Warning, Error, Critical };

class CIUReport {
 public:
  CIUSeverity severity;
  const char* subsystem;
  const char* code;
  const char* message;

  os::utils::ds::HashMap<const char*, const char*> metadataMap;

  CIUReport(CIUSeverity severity, const char* subsystem, const char* code, const char* message)
      : severity(severity), subsystem(subsystem), code(code), message(message) {}

  CIUReport& meta(const char* key, const char* value) {
    metadataMap.Insert(key, value);
    return *this;
    /*
     * returning *this pointer allows for chaining.
     * CIUReport& means return a reference to the same CIUReport object, not a copy of it
     * return *this means "return a reference of myself", the same object that was just modified
     * this allows for syntax like this:
     *   report.meta("phase", "boot")
     *         .meta("category", "init")
     *         .meta("module", "commandregistry");
     *
     * instead of syntax like:
     *   report.meta("phase", "boot");
     *   report.meta("category", "init");
     *   report.meta("module", "commandregistry");
     */
  }
};

}  // namespace ciu
}  // namespace os
#endif
