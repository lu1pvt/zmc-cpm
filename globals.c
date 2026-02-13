#include <stdint.h>


uint8_t CONFIG[] = { // 80x40
    80,  // Columns
    32   // Lines
};

uint8_t DEBUG = 0;

const uint8_t *COLUMNS = CONFIG;
const uint8_t *LINES = CONFIG+1;

