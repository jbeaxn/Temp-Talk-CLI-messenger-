#include "common.h"

extern void delete_volatile_msg();

void handle_alarm(int sig) {
    delete_volatile_msg();
    
}

void init_signal_handler() {
    signal(SIGALRM, handle_alarm);
}

void set_volatile_timer(int sec) {
    alarm(sec); 
}