void check_do_graph() {
  if(has_drawn_graph) {
    has_drawn_graph = 0;
    int key;
    int fkeymenu = 0;
    while(1) {
      DisplayStatusArea();
      if(fkeymenu == 1) drawFkeyLabels(0x005F, 0x0060, 0x0061); // INITIAL, TRIG, STANDRD
      GetKey(&key);
      double xmin, xmax, ymin, ymax, xrange, yrange;
      get_xyminmax(&xmin, &xmax, &ymin, &ymax);
      xrange = xmax - xmin;
      yrange = ymax - ymin;
      if(fkeymenu == 0) {
        if(key == KEY_CTRL_LEFT || key == KEY_CTRL_RIGHT) {
          if(key==KEY_CTRL_LEFT) {
            xmin -= xrange * 0.15;
            xmax -= xrange * 0.15;
          } else {
            xmin += xrange * 0.15;
            xmax += xrange * 0.15;
          }
          // easier than having to set the symbols in the complicated eigenmath system:
          char command[100];
          sprintf(command, "xrange=(%g,%g)", xmin, xmax);
          execution_in_progress = 1;
          has_drawn_graph = 0;
          run(command);
          run(expr);
          execution_in_progress = 0;
        } else if(key == KEY_CTRL_DOWN || key == KEY_CTRL_UP) {
          if(key==KEY_CTRL_DOWN) {
            ymin -= yrange * 0.15;
            ymax -= yrange * 0.15;
          } else {
            ymin += yrange * 0.15;
            ymax += yrange * 0.15;
          }
          // easier than having to set the symbols in the complicated eigenmath system:
          char command[100];
          sprintf(command, "yrange=(%g,%g)", ymin, ymax);
          execution_in_progress = 1;
          has_drawn_graph = 0;
          run(command);
          run(expr);
          execution_in_progress = 0;
        } else if(key == KEY_CHAR_PLUS || key == KEY_CHAR_MINUS) {
          if(key==KEY_CHAR_PLUS) {
            // 0.75 is 3/4
            xmin = xmin * 0.75;
            xmax = xmax * 0.75;
            ymin = ymin * 0.75;
            ymax = ymax * 0.75;
          } else {
            // 1.(3), or 1/(3/4), or 4/3
            xmin = xmin * 4.0/3.0;
            xmax = xmax * 4.0/3.0;
            ymin = ymin * 4.0/3.0;
            ymax = ymax * 4.0/3.0;
          }
          // easier than having to set the symbols in the complicated eigenmath system:
          char command[100];
          sprintf(command, "yrange=(%g,%g)", ymin, ymax);
          execution_in_progress = 1;
          run(command);
          sprintf(command, "xrange=(%g,%g)", xmin, xmax);
          has_drawn_graph = 0;
          run(command);
          run(expr);
          execution_in_progress = 0;
        } else if(key == KEY_CTRL_F3) {
          fkeymenu = 1;
          key = 0;
        } else if(key == KEY_CTRL_EXIT || key==KEY_CTRL_F6) return;
      }
      if(fkeymenu == 1) {
        if(key == KEY_CTRL_F1) {
          execution_in_progress = 1;
          run("xrange=(-10,10)");
          run("yrange=(-10,10)");
          key = KEY_CTRL_EXIT; // redraw and close menu
        } else if(key == KEY_CTRL_F2) {
          execution_in_progress = 1;
          char command[100];
          sprintf(command, "xrange=(%g,%g)", -3.0*M_PI, 3.0*M_PI);
          run(command);
          run("yrange=(-1.6, 1.6)");
          key = KEY_CTRL_EXIT; // redraw and close menu
        } else if(key == KEY_CTRL_F3) {
          execution_in_progress = 1;
          run("xrange=(-10,10)");
          run("yrange=(-5,5)");
          key = KEY_CTRL_EXIT; // redraw and close menu
        }
        if(key == KEY_CTRL_EXIT) { // must not be a "else if"
          fkeymenu = 0;
          execution_in_progress = 1;
          has_drawn_graph = 0;
          run(expr);
          execution_in_progress = 0;
        }
      }
    }
  }
}
