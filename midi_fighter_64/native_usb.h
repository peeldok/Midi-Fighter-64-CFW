#ifndef _NATIVE_USB_H_INCLUDED
#define _NATIVE_USB_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint8_t Event;
    uint8_t Data1;
    uint8_t Data2;
    uint8_t Data3;
} MIDI_EventPacket_t;

enum
{
    DEVICE_STATE_Unattached = 0,
    DEVICE_STATE_Powered,
    DEVICE_STATE_Default,
    DEVICE_STATE_Addressed,
    DEVICE_STATE_Configured,
    DEVICE_STATE_Suspended
};

extern volatile uint8_t USB_DeviceState;

void USB_Init(void);
void USB_Disable(void);
bool MIDI_Device_ReceiveEventPacket(MIDI_EventPacket_t *event);
bool MIDI_Device_SendEventPacket(const MIDI_EventPacket_t *event);
void MIDI_Device_Flush(void);

#endif
