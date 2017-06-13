/*
 * display.h:
 *  Display control functions, such as setting parameters, and indicating that
 *  you are done drawing.
 */

#ifndef LDRAW_DISPLAY_H_
#define LDRAW_DISPLAY_H_

#include <lcommon/geometry.h>

namespace ldraw {

void display_initialize(const char* window_name, const Size& draw_area_size,
		bool fullscreen = false);
void display_set_fullscreen(bool fullscreen);
bool display_is_fullscreen();
void display_set_drawing_region(const BBoxF& bbox);

void display_draw_finish();
void display_draw_start();

Size screen_size();
Size display_size();
Size window_size();

}
        
#endif /* DISPLAY_H_ */
