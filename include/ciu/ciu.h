#ifndef __OS__CIU__CIU_H
#define __OS__CIU__CIU_H

#include <ciu/report.h>
#include <common/types.h>
#include <utils/ds/hashmap.h>
#include <utils/hash.h>

namespace os {
namespace ciu {

struct CIURouteFlags {
  bool toMainTerminal;
  bool toCIUTerminal;  // TODO: implement CIU Terminal
};

struct CIUColor {
  os::utils::VGAColor fg;
  os::utils::VGAColor bg;
};

class CIU {
 private:
  static bool ready;
  static os::utils::ds::HashMap<common::uint32_t, CIURouteFlags> routingMap;
  static os::utils::ds::HashMap<common::uint8_t, CIUColor> colorMap;
  static os::utils::ds::HashMap<common::uint32_t, CIUColor> subsystemColorMap;
  static void SetupDefaultRoutes();
  static void SetupDefaultColors();
  static CIURouteFlags Resolve(const CIUReport& report);
  static CIUColor GetSeverityColor(CIUSeverity severity);
  static CIUColor GetSubsystemColor(const char* subsystemName);
  static common::uint32_t MakeRouteKey(const char* subsystem, CIUSeverity severity);
  static const char* SeverityToString(CIUSeverity severity);
  static void SinkMainTerminal(const CIUReport& report);

 public:
  static void Init();
  static bool IsReady();
  static void Report(const CIUReport& report);
};

}  // namespace ciu
}  // namespace os

#endif
