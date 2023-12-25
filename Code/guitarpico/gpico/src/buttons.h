#ifndef __BUTTONS_H
#define __BUTTONS_H

#ifdef __cplusplus
extern "C"
{
#endif

void buttons_initialize(void);
void buttons_clear(void);
void buttons_poll();
bool button_getpressed(uint8_t b);
bool button_waitpressed(uint8_t b);
uint8_t button_readunbounced(uint8_t b);
uint8_t button_readbutton(uint8_t b);

#ifdef __cplusplus
}
#endif
#endif /* __BUTTONS_H */
