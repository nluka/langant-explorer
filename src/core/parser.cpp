#define _CRT_SECURE_NO_WARNINGS

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "parser.hpp"
#include "event_log.hpp"
#include "../util/util.hpp"
#include "../ui/ui.hpp"
#include "../CommandVars.hpp"
#include "../constants.hpp"

void parse_struct_field(
  FILE * file,
  char const * format,
  void * destination,
  char const * fieldName,
  char const * simName
);

void parse_simulation(FILE * simFile, Simulation * sim);
void parse_simulation_output_format(FILE * simFile, Simulation * sim);

void parse_rule_struct(FILE * simFile, Simulation * sim, size_t ruleCount);
void parse_rule_turn_direction(FILE * simFile, Simulation const * sim, Rule * rule, size_t ruleCount);
void parse_rule_replacement_color(FILE * simFile, Simulation const * sim, Rule * rule, size_t ruleCount);

void parse_ant_initial_orientation(FILE * simFile, Simulation * sim);

void file_move_cursor_to_next_char_occurence(FILE * simFile, char const * searchChar);
char file_move_cursor_to_next_struct_or_array_end(FILE * simFile);
bool file_is_there_another_simulation(FILE * simFile);

size_t parse_simulation_file(Simulation * const sims) {
  FILE * simFile = fopen(g_comVars.simulationFilePathname.c_str(), "r");
  assert_file(simFile, g_comVars.simulationFilePathname.c_str(), true);

  size_t parsedCount = 0;
  do {
    Simulation * sim = &sims[parsedCount];
    parse_simulation(simFile, sim);
    ++parsedCount;
  } while (
    parsedCount < MAX_SIMULATION_COUNT &&
    file_is_there_another_simulation(simFile)
  );

  fclose(simFile);

  log_event(
    EventType::Success,
    "Parsed simulation file '%s'",
    g_comVars.simulationFilePathname.c_str()
  );

  return parsedCount;
}

void parse_simulation(FILE * const simFile, Simulation * const sim) {
  {
    file_move_cursor_to_next_char_occurence(simFile, "(");
    parse_struct_field(simFile, "%255[^,], ", sim->name, "simulation name", NULL);
    parse_simulation_output_format(simFile, sim);
    parse_struct_field(
      simFile,
      "%" PRIu64 ")",
      &sim->maxIterationsCount,
      "simulation max-iterations",
      sim->name
    );
  }

  { // grid
    file_move_cursor_to_next_char_occurence(simFile, "(");

    parse_struct_field(
      simFile,
      "%" PRIu16 ", ",
      &sim->grid.width,
      "grid width",
      sim->name
    );

    parse_struct_field(
      simFile,
      "%" PRIu16 ", ",
      &sim->grid.height,
      "grid height",
      sim->name
    );

    parse_struct_field(
      simFile,
      "%" PRIu8 ")",
      &sim->grid.initialColor,
      "grid initial-color",
      sim->name
    );
  }

  { // ruleset
    size_t ruleCount = 0;

    file_move_cursor_to_next_char_occurence(simFile, "[");
    file_move_cursor_to_next_char_occurence(simFile, "(");

    char c;
    do {
      parse_rule_struct(simFile, sim, ruleCount);
      ++ruleCount;
      c = file_move_cursor_to_next_struct_or_array_end(simFile);
    } while (c != ']');

    ruleset_validate(&sim->ruleset, &sim->grid, sim->name);
  }

  { // ant
    file_move_cursor_to_next_char_occurence(simFile, "(");

    parse_struct_field(
      simFile,
      "%" PRIu16 ", ",
      &sim->ant.col,
      "ant initial-column",
      sim->name
    );

    parse_struct_field(
      simFile,
      "%" PRIu16 ", ",
      &sim->ant.row,
      "ant initial-row",
      sim->name
    );

    parse_ant_initial_orientation(simFile, sim);

    ant_validate(&sim->ant, &sim->grid, sim->name);
  }
}

void file_move_cursor_to_next_char_occurence(
  FILE * const simFile,
  char const * const searchChar
) {
  int c;

  do {
    c = fgetc(simFile);
    if (c == EOF) {
      log_event(EventType::Error, "Failed to parse: expected '%s'", searchChar);
      printfc(TextColor::Red, "Failed to parse: expected '%s'", searchChar);
      printf("\n");
      end(ExitCode::FailedToParseSimulationFile);
    }
  } while (c != searchChar[0]);
}

char file_move_cursor_to_next_struct_or_array_end(FILE * const simFile) {
  int c;

  do {
    c = fgetc(simFile);
    if (c == EOF) {
      char const * const format = "Failed to parse: expected '(' or ']'";
      log_event(EventType::Error, format);
      printfc(TextColor::Red, format);
      printf("\n");
      end(ExitCode::FailedToParseSimulationFile);
    }
  } while (c != '(' && c != ']');
  return (char)c;
}

void parse_struct_field(
  FILE * const simFile,
  char const * const format,
  void * const destination,
  char const * const fieldName,
  char const * const simName
) {
  if (fscanf(simFile, format, destination) == 1) {
    // scan successful
    return;
  }

  if (simName != NULL) {
    char const * const errFormat = "Failed to parse simulation '%s': unable to read %s";
    log_event(EventType::Error, errFormat, simName, fieldName);
    printfc(TextColor::Red, errFormat, simName, fieldName);
  } else {
    char const * const errFormat = "Failed to parse simulation: unable to read %s";
    log_event(EventType::Error, errFormat, fieldName);
    printfc(TextColor::Red, errFormat, fieldName);
  }
  printf("\n");

  end(ExitCode::FailedToParseSimulationFile);
}

void parse_simulation_output_format(FILE * const simFile, Simulation * const sim) {
  char outputFormat[5] = { 0 };

  parse_struct_field(
    simFile,
    "%4[^,], ",
    outputFormat,
    "simulation output-format",
    sim->name
  );

  if (are_strings_identical(outputFormat, "PGMa")) {
    sim->imageFileType = SimulationImageFileType::PgmAscii;
  } else if (are_strings_identical(outputFormat, "PGMb")) {
    sim->imageFileType = SimulationImageFileType::PgmBinary;
  } else {
    char const * const errFormat =
      "Parsing of simulation '%s' failed: invalid simulation output-format";
    log_event(EventType::Error, errFormat, sim->name);
    printfc(TextColor::Red, errFormat, sim->name);
    printf("\n");
    end(ExitCode::FailedToParseSimulationFile);
  }
}

void parse_rule_struct(
  FILE * const simFile,
  Simulation * const sim,
  size_t const ruleCount
) {
  Rule rule;
  color_t color;

  {
    char fieldName[] = "rule x color";
    fieldName[5] = '0' + static_cast<char>(ruleCount + 1);
    parse_struct_field(
      simFile,
      "%" PRIu8 ", ",
      &color,
      fieldName,
      sim->name
    );
  }

  parse_rule_turn_direction(simFile, sim, &rule, ruleCount);
  parse_rule_replacement_color(simFile, sim, &rule, ruleCount);

  sim->ruleset.rules[color] = rule;
  sim->ruleset.rules[color].isUsed = true;
}

void parse_rule_turn_direction(
  FILE * const simFile,
  Simulation const * const sim,
  Rule * const rule,
  size_t const ruleCount
) {
  char fieldName[] = "rule x turn-direction", turnDirectionStr[6] = { 0 };

  char const ruleNumber = '0' + static_cast<char>(ruleCount + 1);
  fieldName[5] = ruleNumber;

  parse_struct_field(simFile, "%5[^,], ", turnDirectionStr, fieldName, sim->name);

  if (are_strings_identical(turnDirectionStr, "left")) {
    rule->turnDirection = RTD_LEFT;
  } else if (are_strings_identical(turnDirectionStr, "none")) {
    rule->turnDirection = RTD_NONE;
  } else if (are_strings_identical(turnDirectionStr, "right")) {
    rule->turnDirection = RTD_RIGHT;
  } else {
    char const * const errFormat =
      "Failed to parse simulation '%s': invalid 'turn-direction' for rule #" PRIu8;
    log_event(EventType::Error, errFormat, sim->name, ruleNumber);
    printfc(TextColor::Red, errFormat, sim->name, ruleNumber);
    printf("\n");
    end(ExitCode::FailedToParseSimulationFile);
  }
}

void parse_rule_replacement_color(
  FILE * const simFile,
  Simulation const * const sim,
  Rule * const rule,
  size_t const ruleCount
) {
  char fieldName[] = "rule x replacement-color";
  fieldName[5] = '0' + static_cast<char>(ruleCount + 1);

  parse_struct_field(
    simFile,
    "%" PRIu8 ")",
    &rule->replacementColor,
    fieldName,
    sim->name
  );
}

void parse_ant_initial_orientation(FILE * const simFile, Simulation * const sim) {
  Ant * const ant = &sim->ant;
  char orientationStr[6] = { 0 };

  parse_struct_field(
    simFile,
    "%5[^)]",
    orientationStr,
    "ant initial-orientation",
    sim->name
  );

  if (are_strings_identical(orientationStr, "north")) {
    ant->orientation = AO_NORTH;
  } else if (are_strings_identical(orientationStr, "east")) {
    ant->orientation = AO_EAST;
  } else if (are_strings_identical(orientationStr, "south")) {
    ant->orientation = AO_SOUTH;
  } else if (are_strings_identical(orientationStr, "west")) {
    ant->orientation = AO_WEST;
  } else {
    char const * const errFormat =
      "Parsing of simluation '%s' failed: invalid ant initial-orientation";
    log_event(EventType::Error, errFormat, sim->name);
    printfc(TextColor::Red, errFormat, sim->name);
    printf("\n");
    end(ExitCode::FailedToParseSimulationFile);
  }
}

bool file_is_there_another_simulation(FILE * const simFile) {
  int c;

  while (1) {
    c = fgetc(simFile);
    if (c == '#') {
      return true;
    }
    if (c == EOF) {
      return false;
    }
  }
}
