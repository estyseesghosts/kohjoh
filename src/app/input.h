#ifndef KJ_APP_INPUT_H
#define KJ_APP_INPUT_H

void kj_input_init(void);
/* Read the buttons and touch screen once per V-blank. */
void kj_input_sample(void);

#endif
