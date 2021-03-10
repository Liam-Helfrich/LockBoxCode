#ifndef SCREEN_COMMANDS_H
#define SCREEN_COMMANDS_H

void printCenter(const char* msg, uint8_t y_coord = 32){
  ssd1306_printFixed((128-ssd1306_getTextSize(msg, NULL))/2, y_coord, msg, STYLE_NORMAL);
}

void printRelative(const char* msg, float x_coord, float y_coord){
  lcduint_t* text_y_size;
  lcduint_t text_x_size = ssd1306_getTextSize(msg, text_y_size);
  ssd1306_printFixed(128*x_coord - text_x_size/2, 64*y_coord - (*text_y_size)/2, msg, STYLE_NORMAL);
}
#endif
