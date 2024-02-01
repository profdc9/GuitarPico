#ifndef _USBMAIN_H
#define _USBMAIN_H

#ifdef __cplusplus
extern "C" {
#endif

int usb_init(void);
void cdc_task(void);
void midi_task(void);
void usb_write_char(uint8_t ch);
int usb_read_character(void);
void usb_task(void);
void midi_send_note(uint8_t note, uint8_t velocity);

#ifdef __cplusplus
}
#endif


#endif /* _USBMAIN_H */
