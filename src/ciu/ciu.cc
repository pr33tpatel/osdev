#include <ciu/ciu.h>

using namespace os;
using namespace os::common;
using namespace os::ciu;
using namespace os::utils;

bool CIU::ready = false;
ds::HashMap<uint32_t, CIURouteFlags> CIU::routingMap;
ds::HashMap<uint8_t, CIUColor> CIU::colorMap;


void CIU::Init() {
  SetupDefaultRoutes();
  SetupDefaultColors();
  ready = true;
}


bool CIU::IsReady() {
  return ready;
}


void CIU::Report(const CIUReport& report) {
  if (!ready) {
    printf(RED_COLOR, BLACK_COLOR, "[CIU] CIU is not ready. %s: %s\n", report.code, report.message);
    return;
  }

  CIURouteFlags flags = Resolve(report);
  if (flags.toMainTerminal) {
    SinkMainTerminal(report);
  }
  // TODO: after CIU terminal is implemented, handle flags.toCIUTerminal
}


void CIU::SetupDefaultRoutes() {
  /* Default Policy: what content gets displayed by the severity
   * Trace    -> CIU terminal only (deferred, not implemented yet)
   * Info     -> CIU terminal only (deferred)
   * Warning  -> main + CIU terminal
   * Error    -> main + CIU terminal
   * Critical -> main + CIU terminal
   */

  routingMap.Insert(MakeRouteKey("*", CIUSeverity::Trace), {false, true});
  routingMap.Insert(MakeRouteKey("*", CIUSeverity::Info), {false, true});
  routingMap.Insert(MakeRouteKey("*", CIUSeverity::Warning), {true, true});
  routingMap.Insert(MakeRouteKey("*", CIUSeverity::Error), {true, true});
  routingMap.Insert(MakeRouteKey("*", CIUSeverity::Critical), {true, true});
}


void CIU::SetupDefaultColors() {
  colorMap.Insert((uint8_t)CIUSeverity::Trace, {DARK_GRAY_COLOR, BLACK_COLOR});
  colorMap.Insert((uint8_t)CIUSeverity::Info, {LIGHT_GRAY_COLOR, BLACK_COLOR});
  colorMap.Insert((uint8_t)CIUSeverity::Warning, {YELLOW_COLOR, BLACK_COLOR});
  colorMap.Insert((uint8_t)CIUSeverity::Error, {LIGHT_RED_COLOR, BLACK_COLOR});
  colorMap.Insert((uint8_t)CIUSeverity::Critical, {WHITE_COLOR, RED_COLOR});
}


CIURouteFlags CIU::Resolve(const CIUReport& report) {
  CIURouteFlags flags;

  uint32_t specificKey = MakeRouteKey(report.subsystem, report.severity);
  if (routingMap.Get(specificKey, flags)) {
    return flags;
  }
  uint32_t wildcardKey = MakeRouteKey("*", report.severity);
  if (routingMap.Get(wildcardKey, flags)) {
    return flags;
  }
  printf("[CIU][DEBUG] no route found, returning false/false\n");
  return {false, false};
}


CIUColor CIU::GetSeverityColor(CIUSeverity severity) {
  CIUColor color;
  if (colorMap.Get((uint8_t)severity, color)) {
    return color;
  }
  return {LIGHT_GRAY_COLOR, BLACK_COLOR};
}


uint32_t CIU::MakeRouteKey(const char* subsystem, CIUSeverity severity) {
  uint32_t h = Hasher<const char*>::Hash(subsystem);
  // mix in severity into the hashed key
  h ^= (uint32_t)severity * 2654435761U;  // ALGORITHM: LZ4 hash function
  return h;
}


const char* CIU::SeverityToString(CIUSeverity severity) {
  switch (severity) {
      // clang-format off
    case CIUSeverity::Trace:     return "TRACE";
    case CIUSeverity::Info:      return "INFO";
    case CIUSeverity::Warning:   return "WARNING";
    case CIUSeverity::Error:     return "ERROR";
    case CIUSeverity::Critical:  return "CRITICAL";

    default:                     return "UNKNOWN";

      // clang-format on
  }
}


void CIU::SinkMainTerminal(const CIUReport& report) {
  /* Format := [SUBSYSTEM][SEVERITY] message (code) */

  CIUColor severityColor = GetSeverityColor(report.severity);
  CIUColor labelColor = {LIGHT_GRAY_COLOR, BLACK_COLOR};

  printf(labelColor.fg, labelColor.bg, "[%s]", report.subsystem);
  printf(severityColor.fg, severityColor.bg, "[%s]:", SeverityToString(report.severity));
  printf(severityColor.fg, severityColor.bg, "%s", report.message);
  printf(labelColor.fg, labelColor.bg, " (%s)\n", report.code);
}
