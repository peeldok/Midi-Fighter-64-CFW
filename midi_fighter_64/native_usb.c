#include "native_usb.h"
#include "usb_descriptors.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

volatile uint8_t USB_DeviceState = DEVICE_STATE_Unattached;
static volatile uint8_t usb_configuration = 0;

void EVENT_USB_Device_Connect(void) __attribute__((weak));
void EVENT_USB_Device_Disconnect(void) __attribute__((weak));
void EVENT_USB_Device_ConfigurationChanged(void) __attribute__((weak));

#define EP0_SIZE 64
#define EP_SINGLE_64 ((1U << ALLOC) | (1U << EPSIZE1) | (1U << EPSIZE0))
#define EP_DOUBLE_64 ((1U << ALLOC) | (1U << EPSIZE1) | (1U << EPSIZE0) | (1U << EPBK0))
#define EP_TYPE_CONTROL_NATIVE 0
#define EP_TYPE_BULK_OUT_NATIVE (1U << EPTYPE1)
#define EP_TYPE_BULK_IN_NATIVE ((1U << EPTYPE1) | (1U << EPDIR))

#define USB_REQ_GET_STATUS 0x00
#define USB_REQ_CLEAR_FEATURE 0x01
#define USB_REQ_SET_FEATURE 0x03
#define USB_REQ_SET_ADDRESS 0x05
#define USB_REQ_GET_DESCRIPTOR 0x06
#define USB_REQ_GET_CONFIGURATION 0x08
#define USB_REQ_SET_CONFIGURATION 0x09
#define USB_REQ_GET_INTERFACE 0x0A
#define USB_REQ_SET_INTERFACE 0x0B

#define USB_REQTYPE_DIR_IN 0x80
#define USB_REQTYPE_TYPE_MASK 0x60
#define USB_REQTYPE_STANDARD 0x00
#define USB_REQTYPE_RECIP_MASK 0x1F
#define USB_REQTYPE_RECIP_DEVICE 0x00

#define USB_DESC_DEVICE 0x01
#define USB_DESC_CONFIGURATION 0x02
#define USB_DESC_STRING 0x03

#define USB_FEATURE_ENDPOINT_HALT 0x00
#define USB_FEATURE_REMOTE_WAKEUP 0x01

#define USB_RELEASE_RX_VALUE 0x6B
#define USB_RELEASE_TX_VALUE 0x3A

typedef struct __attribute__((packed))
{
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint8_t wValueL;
    uint8_t wValueH;
    uint16_t wIndex;
    uint16_t wLength;
} usb_setup_t;

static uint16_t control_mark;
static uint16_t control_end;

static inline void usb_select_endpoint(uint8_t ep)
{
    UENUM = ep;
}

static inline void usb_clear_in(void)
{
    UEINTX = (uint8_t)~(1U << TXINI);
}

static inline void usb_clear_setup(void)
{
    UEINTX = (uint8_t)~((1U << RXSTPI) | (1U << RXOUTI) | (1U << TXINI));
}

static inline void usb_stall(void)
{
    UECONX = (1U << STALLRQ) | (1U << EPEN);
}

static inline void usb_release_rx(void)
{
    UEINTX = USB_RELEASE_RX_VALUE;
}

static inline void usb_release_tx(void)
{
    UEINTX = USB_RELEASE_TX_VALUE;
}

static void usb_init_endpoint(uint8_t ep, uint8_t type, uint8_t size)
{
    usb_select_endpoint(ep);
    UECONX = (1U << EPEN);
    UECFG0X = type;
    UECFG1X = size;
}

static bool usb_configure_midi_endpoints(void)
{
    uint8_t saved_sreg = SREG;
    cli();
    uint8_t saved_endpoint = UENUM;

    UERST = (1U << MIDI_STREAM_OUT_EPNUM) | (1U << MIDI_STREAM_IN_EPNUM);
    UERST = 0;

    usb_init_endpoint(MIDI_STREAM_OUT_EPNUM, EP_TYPE_BULK_OUT_NATIVE, EP_DOUBLE_64);
    bool out_ok = (UESTA0X & (1U << CFGOK)) != 0;

    usb_init_endpoint(MIDI_STREAM_IN_EPNUM, EP_TYPE_BULK_IN_NATIVE, EP_DOUBLE_64);
    bool in_ok = (UESTA0X & (1U << CFGOK)) != 0;

    UENUM = saved_endpoint;
    SREG = saved_sreg;
    return out_ok && in_ok;
}

static inline bool usb_wait_in_or_out(void)
{
    while (!(UEINTX & ((1U << TXINI) | (1U << RXOUTI)))) {
    }
    return (UEINTX & (1U << RXOUTI)) == 0;
}

static void control_init(uint16_t end)
{
    usb_select_endpoint(0);
    control_mark = 0;
    control_end = end;
}

static bool control_send_byte(uint8_t value)
{
    if (control_mark < control_end) {
        if (!usb_wait_in_or_out()) return false;
        UEDATX = value;
        if (((control_mark + 1U) & (EP0_SIZE - 1U)) == 0U) {
            usb_clear_in();
        }
    }
    control_mark++;
    return true;
}

static bool control_send_progmem(const uint8_t *data, uint16_t length)
{
    while (length--) {
        if (!control_send_byte(pgm_read_byte(data++))) return false;
    }
    return true;
}

static bool control_send_descriptor(const usb_setup_t *setup)
{
    uint16_t size = 0;
    const uint8_t *descriptor = usb_descriptor_get(setup->wValueH, setup->wValueL, &size);
    if (!descriptor || size == 0) return false;

    control_init(setup->wLength);
    return control_send_progmem(descriptor, size);
}

static void usb_clock_enable(void)
{
#if defined(UHWCON) && defined(UVREGE)
    UHWCON |= (1U << UVREGE);
#endif

    USBCON = (1U << USBE) | (1U << FRZCLK);

#if F_CPU == 16000000UL
    PLLCSR |= (1U << PINDIV);
#elif F_CPU == 8000000UL
    PLLCSR &= ~(1U << PINDIV);
#else
#error Native USB supports 8 MHz or 16 MHz ATmega32U4 clocks only
#endif

    PLLCSR |= (1U << PLLE);
    while (!(PLLCSR & (1U << PLOCK))) {
    }

    _delay_ms(1);

#if defined(OTGPADE)
    USBCON = (USBCON & ~(1U << FRZCLK)) | (1U << OTGPADE);
#else
    USBCON &= ~(1U << FRZCLK);
#endif

#if defined(LSM)
    UDCON &= ~(1U << LSM);
#endif
    UDCON &= ~((1U << RMWKUP) | (1U << DETACH));
}

void USB_Init(void)
{
    uint8_t saved_sreg = SREG;
    cli();

    usb_configuration = 0;
    USB_DeviceState = DEVICE_STATE_Powered;

    UDCON |= (1U << DETACH);
    usb_clock_enable();

    UDINT = 0;
    UDIEN = (1U << EORSTE);

    SREG = saved_sreg;

    if (EVENT_USB_Device_Connect) EVENT_USB_Device_Connect();
}

void USB_Disable(void)
{
    uint8_t saved_sreg = SREG;
    cli();

    UDIEN = 0;
    UDCON |= (1U << DETACH);
    USBCON |= (1U << FRZCLK);
    PLLCSR &= ~(1U << PLLE);
#if defined(UHWCON) && defined(UVREGE)
    UHWCON &= ~(1U << UVREGE);
#endif
    USB_DeviceState = DEVICE_STATE_Unattached;
    usb_configuration = 0;

    SREG = saved_sreg;

    if (EVENT_USB_Device_Disconnect) EVENT_USB_Device_Disconnect();
}

bool MIDI_Device_ReceiveEventPacket(MIDI_EventPacket_t *event)
{
    if (!event || USB_DeviceState != DEVICE_STATE_Configured) return false;

    uint8_t saved_sreg = SREG;
    cli();
    uint8_t saved_endpoint = UENUM;
    usb_select_endpoint(MIDI_STREAM_OUT_EPNUM);

    bool received = false;
    if ((UEINTX & (1U << RXOUTI)) && UEBCLX >= sizeof(MIDI_EventPacket_t)) {
        event->Event = UEDATX;
        event->Data1 = UEDATX;
        event->Data2 = UEDATX;
        event->Data3 = UEDATX;
        received = true;

        if (UEBCLX == 0) {
            usb_release_rx();
        }
    } else if ((UEINTX & (1U << RXOUTI)) && UEBCLX == 0) {
        usb_release_rx();
    }

    UENUM = saved_endpoint;
    SREG = saved_sreg;
    return received;
}

bool MIDI_Device_SendEventPacket(const MIDI_EventPacket_t *event)
{
    if (!event || USB_DeviceState != DEVICE_STATE_Configured) return false;

    uint8_t start_frame = UDFNUML;
    uint16_t spins = 0;

    while (spins++ < 24000U) {
        uint8_t saved_sreg = SREG;
        cli();
        uint8_t saved_endpoint = UENUM;
        usb_select_endpoint(MIDI_STREAM_IN_EPNUM);

        if (UEINTX & (1U << RWAL)) {
            UEDATX = event->Event;
            UEDATX = event->Data1;
            UEDATX = event->Data2;
            UEDATX = event->Data3;

            if (!(UEINTX & (1U << RWAL))) {
                usb_release_tx();
            }

            UENUM = saved_endpoint;
            SREG = saved_sreg;
            return true;
        }

        UENUM = saved_endpoint;
        SREG = saved_sreg;

        if ((uint8_t)(UDFNUML - start_frame) >= 2U) break;
    }

    return false;
}

void MIDI_Device_Flush(void)
{
    if (USB_DeviceState != DEVICE_STATE_Configured) return;

    uint8_t saved_sreg = SREG;
    cli();
    uint8_t saved_endpoint = UENUM;
    usb_select_endpoint(MIDI_STREAM_IN_EPNUM);

    if (UEBCLX != 0) {
        usb_release_tx();
    }

    UENUM = saved_endpoint;
    SREG = saved_sreg;
}

ISR(USB_GEN_vect)
{
    uint8_t flags = UDINT;

    if (flags & (1U << EORSTI)) {
        UDINT &= ~(1U << EORSTI);

        usb_init_endpoint(0, EP_TYPE_CONTROL_NATIVE, EP_SINGLE_64);
        UEIENX = (1U << RXSTPE);

        usb_configuration = 0;
        USB_DeviceState = DEVICE_STATE_Default;
    }
}

ISR(USB_COM_vect)
{
    usb_select_endpoint(0);
    if (!(UEINTX & (1U << RXSTPI))) return;

    usb_setup_t setup;
    uint8_t *raw = (uint8_t *)&setup;
    for (uint8_t i = 0; i < sizeof(setup); i++) raw[i] = UEDATX;
    usb_clear_setup();

    uint8_t request_type = setup.bmRequestType;
    if (request_type & USB_REQTYPE_DIR_IN) {
        while (!(UEINTX & (1U << TXINI))) {
        }
    } else {
        usb_clear_in();
    }

    bool ok = true;

    if ((request_type & USB_REQTYPE_TYPE_MASK) == USB_REQTYPE_STANDARD) {
        uint16_t w_value = (uint16_t)setup.wValueL | ((uint16_t)setup.wValueH << 8);

        switch (setup.bRequest) {
            case USB_REQ_GET_STATUS:
                UEDATX = 0;
                UEDATX = 0;
                break;

            case USB_REQ_CLEAR_FEATURE:
                if ((request_type & USB_REQTYPE_RECIP_MASK) == USB_REQTYPE_RECIP_DEVICE &&
                    w_value == USB_FEATURE_REMOTE_WAKEUP) {
                    ok = true;
                } else if ((request_type & USB_REQTYPE_RECIP_MASK) == 0x02 &&
                           w_value == USB_FEATURE_ENDPOINT_HALT) {
                    uint8_t saved_endpoint = UENUM;
                    usb_select_endpoint((uint8_t)(setup.wIndex & 0x0F));
                    UECONX |= (1U << RSTDT);
                    UECONX &= ~(1U << STALLRQ);
                    UENUM = saved_endpoint;
                } else {
                    ok = false;
                }
                break;

            case USB_REQ_SET_FEATURE:
                if ((request_type & USB_REQTYPE_RECIP_MASK) == USB_REQTYPE_RECIP_DEVICE &&
                    w_value == USB_FEATURE_REMOTE_WAKEUP) {
                    ok = true;
                } else {
                    ok = false;
                }
                break;

            case USB_REQ_SET_ADDRESS:
                while (!(UEINTX & (1U << TXINI))) {
                }
                UDADDR = setup.wValueL | (1U << ADDEN);
                USB_DeviceState = setup.wValueL ? DEVICE_STATE_Addressed : DEVICE_STATE_Default;
                break;

            case USB_REQ_GET_DESCRIPTOR:
                ok = control_send_descriptor(&setup);
                break;

            case USB_REQ_GET_CONFIGURATION:
                UEDATX = usb_configuration;
                break;

            case USB_REQ_SET_CONFIGURATION:
                if ((request_type & USB_REQTYPE_RECIP_MASK) != USB_REQTYPE_RECIP_DEVICE || setup.wValueL > 1U) {
                    ok = false;
                    break;
                }

                usb_configuration = setup.wValueL;
                if (usb_configuration) {
                    if (!usb_configure_midi_endpoints()) {
                        ok = false;
                        usb_configuration = 0;
                        USB_DeviceState = DEVICE_STATE_Addressed;
                    } else {
                        USB_DeviceState = DEVICE_STATE_Configured;
                        if (EVENT_USB_Device_ConfigurationChanged) EVENT_USB_Device_ConfigurationChanged();
                    }
                } else {
                    USB_DeviceState = DEVICE_STATE_Addressed;
                }
                usb_select_endpoint(0);
                break;

            case USB_REQ_GET_INTERFACE:
                UEDATX = 0;
                break;

            case USB_REQ_SET_INTERFACE:
                if (setup.wValueL != 0) ok = false;
                break;

            default:
                ok = false;
                break;
        }
    } else {
        ok = false;
    }

    usb_select_endpoint(0);
    if (ok) {
        usb_clear_in();
    } else {
        usb_stall();
    }
}
