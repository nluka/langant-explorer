#include <stdlib.h>
#include "Grid.hpp"
#include "../util/util.hpp"

bool grid_init_cells(Grid * const grid) {
  size_t const cellCount = grid->width * grid->height;

  // try to allocate
  grid->cells = (color_t*)malloc(sizeof(*grid->cells) * cellCount);
  if (grid->cells == NULL) {
    return false;
  }

  // init cells
  for (size_t i = 0; i < cellCount; ++i) {
    grid->cells[i] = grid->initialColor;
  }

  grid->cellCount = cellCount;

  return true;
}

bool grid_is_col_in_bounds(Grid const * const grid, int const col) {
  return col >= 0 && col < (grid->width - 1);
}

bool grid_is_row_in_bounds(Grid const * const grid, int const row) {
  return row >= 0 && row < (grid->height - 1);
}

size_t grid_get_cell_index(
  Grid const * const grid,
  uint16_t const col,
  uint16_t const row
) {
  return (row * grid->width) + col;
}

void grid_destroy(Grid * const grid) {
  free(grid->cells);
  grid->cells = NULL;
  grid->width = 0;
  grid->height = 0;
  grid->cellCount = 0;
}
