#ifndef KEY_DETECTOR_H
#define KEY_DETECTOR_H

/*definition de l'etat du programme*/
typedef struct {
    int is_running;
    char last_key_pressed[128];
} programme_state;

WINDOW *initialize_ncurses();

void draw_ui(WINDOW *work, programme_state *state);
void handle_input(programme_state *state);

#endif