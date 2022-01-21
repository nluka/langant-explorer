#ifndef ANT_SIMULATOR_LITE_CORE_GRID_HPP
#define ANT_SIMULATOR_LITE_CORE_GRID_HPP

#include <stdlib.h>
#include <stdbool.h>
#include "types.hpp"

struct Grid {
  uint16_t  width;
  color_t   initialColor;
  color_t * cells;
  uint16_t  height;
  size_t    cellCount;
};

bool   grid_init_cells(Grid * grid);
bool   grid_is_col_in_bounds(Grid const * grid, int col);
bool   grid_is_row_in_bounds(Grid const * grid, int row);
size_t grid_get_cell_index(Grid const * grid, uint16_t col, uint16_t row);
void   grid_destroy(Grid * grid);

#endif // ANT_SIMULATOR_LITE_CORE_GRID_HPP
