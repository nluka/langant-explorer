#include <stdbool.h>
#include <inttypes.h>
#include "Ruleset.hpp"
#include "event_log.hpp"
#include "../ui/ui.hpp"
#include "../util/util.hpp"

void ruleset_validate(
  Ruleset const * const ruleset,
  Grid const * const grid,
  char const * const simName
) {
  if (!ruleset->rules[grid->initialColor].isUsed) {
    char const * const format =
      "Parsing of simulation '%s' failed: no rules govern grid initial-color";
    log_event(EventType::Error, format, simName);
    printfc(TextColor::Red, format, simName);
    printf("\n");
    end(ExitCode::FailedToParseSimulationFile);
  }

  for (uint8_t i = 0; i < UINT8_MAX; ++i) {
    Rule const * const rule = &ruleset->rules[i];

    if (!rule->isUsed) {
      continue;
    }

    Rule const * const governingRule =
      &ruleset->rules[rule->replacementColor];

    if (!governingRule->isUsed) {
      const char * const format =
        "Parsing of simulation '%s' failed: rule for color %" PRIu8
        " has replacement-color %" PRIu8 " which has no governing rule";
      log_event(EventType::Error, format, simName, i, rule->replacementColor);
      printfc(TextColor::Red, format, simName, i, rule->replacementColor);
      printf("\n");
      end(ExitCode::FailedToParseSimulationFile);
    }
  }
}

color_t ruleset_find_largest_color(Ruleset const * const ruleset) {
  for (uint8_t color = UINT8_MAX; color > 0; --color) {
    if (ruleset->rules[color].isUsed) {
      return color;
    }
  }
  return 0;
}
