#include "sysex.h"
#include "constants.h"
#include "midi.h"
#include "led.h"
#include "fastrgb.h"
#include "rotation.h"
#include "eeprom.h"
#include <stdbool.h>

typedef enum {
    State_Begin = 0,
    State_CheckMID,
    State_Invalid,
    State_NonRealtime,
    State_DJTT,
    State_WebPrefix,
    State_Web,
    State_6F,
    State_5F
} SysexState;

typedef struct {
    uint8_t buffer[MIDI_MAX_SYSEX];
    uint16_t length;
    SysexState state;
    bool reading;
} SysexContext;

static SysexContext sysex_ctx;
static uint8_t sysex_owner_cable = 0xFF;

#define MAX_COMMAND 8
SysExFn sysExCommandMap[MAX_COMMAND] = {0,};

// Web SysEx prefix: F0 7D 4D 46 36 34 ... F7
// DISCOVER request 0x01 + token -> DISCOVER_REPLY 0x02 + token + protocol + firmware version + capabilities.
// GET_SETTINGS request 0x03 -> SETTINGS_REPLY 0x04 + rotation + MIDI velocity.
// SET_ROTATION request 0x05 + rotation -> saves rotation and returns SETTINGS_REPLY 0x04.
// SET_VELOCITY request 0x06 + velocity -> saves velocity and returns SETTINGS_REPLY 0x04.
// PALETTE_UPLOAD request 0x10 + component + 128 x 6-bit values -> PALETTE_ACK 0x11 + component + status.
// PALETTE_DOWNLOAD request 0x12 + component -> PALETTE_DATA 0x13 + component + 128 x 6-bit values.
#define WEB_PROTOCOL_VERSION 2
#define WEB_CAP_ROTATION 0x01
#define WEB_CAP_CUSTOM_PALETTE 0x02
#define WEB_CAP_PALETTE_6BIT 0x04
#define WEB_CAP_VELOCITY 0x10
#define WEB_CMD_DISCOVER 0x01
#define WEB_CMD_DISCOVER_REPLY 0x02
#define WEB_CMD_GET_SETTINGS 0x03
#define WEB_CMD_SETTINGS_REPLY 0x04
#define WEB_CMD_SET_ROTATION 0x05
#define WEB_CMD_SET_VELOCITY 0x06
#define WEB_CMD_PALETTE_UPLOAD 0x10
#define WEB_CMD_PALETTE_ACK 0x11
#define WEB_CMD_PALETTE_DOWNLOAD 0x12
#define WEB_CMD_PALETTE_DATA 0x13
#define WEB_STATUS_OK 0x00
#define WEB_STATUS_INVALID 0x01

static inline uint8_t sysex_packet_cable(const MIDI_EventPacket_t* packet)
{
    return (uint8_t)((packet->Event >> 4) & 0x01);
}

static inline bool sysex_accept_packet(const MIDI_EventPacket_t* packet)
{
    if (!sysex_ctx.reading) return true;
    return sysex_owner_cable == sysex_packet_cable(packet);
}

static inline void sysex_reset(SysexContext* ctx)
{
    ctx->length = 0;
    ctx->state = State_Begin;
    ctx->reading = false;
    sysex_owner_cable = 0xFF;
}

static inline bool sysex_append(SysexContext* ctx, uint8_t value)
{
    if (ctx->length >= MIDI_MAX_SYSEX) {
        ctx->state = State_Invalid;
        return false;
    }
    ctx->buffer[ctx->length++] = value;
    return true;
}

static void web_send_settings(uint8_t cable)
{
    uint8_t msg[] = {
        0xF0,0x7D,0x4D,0x46,0x36,0x34,WEB_CMD_SETTINGS_REPLY,
        rotation_get(),G_EE_MIDI_VELOCITY,0xF7
    };
    midi_stream_sysex_cable(sizeof(msg), msg, cable);
}

static void web_send_discovery(uint8_t cable, uint8_t token)
{
    uint8_t msg[] = {
        0xF0,0x7D,0x4D,0x46,0x36,0x34,WEB_CMD_DISCOVER_REPLY,
        token,WEB_PROTOCOL_VERSION,1,0,1,
        WEB_CAP_ROTATION | WEB_CAP_CUSTOM_PALETTE | WEB_CAP_PALETTE_6BIT | WEB_CAP_VELOCITY,
        0xF7
    };
    midi_stream_sysex_cable(sizeof(msg), msg, cable);
}

static void web_send_palette_ack(uint8_t cable, uint8_t component, uint8_t status)
{
    uint8_t msg[] = {
        0xF0,0x7D,0x4D,0x46,0x36,0x34,WEB_CMD_PALETTE_ACK,
        component,status,0xF7
    };
    midi_stream_sysex_cable(sizeof(msg), msg, cable);
}

static void web_send_palette_component(SysexContext* ctx, uint8_t cable, uint8_t component)
{
    uint16_t pos = 0;
    if (component > 2) return;

    ctx->buffer[pos++] = 0xF0;
    ctx->buffer[pos++] = 0x7D;
    ctx->buffer[pos++] = 0x4D;
    ctx->buffer[pos++] = 0x46;
    ctx->buffer[pos++] = 0x36;
    ctx->buffer[pos++] = 0x34;
    ctx->buffer[pos++] = WEB_CMD_PALETTE_DATA;
    ctx->buffer[pos++] = component;

    for (uint16_t i = 0; i < 128; i++) {
        ctx->buffer[pos++] = fastrgb_ch4_read_component6(component, (uint8_t)i);
    }

    ctx->buffer[pos++] = 0xF7;
    midi_stream_sysex_cable(pos, ctx->buffer, cable);
}

static void web_sysex_handle(SysexContext* ctx, uint8_t cable, uint16_t length)
{
    uint8_t* buffer = ctx->buffer;
    if (length < 2 || buffer[length - 1] != 0xF7) return;

    uint8_t command = buffer[0];

    if (command == WEB_CMD_DISCOVER) {
        if (length == 3) web_send_discovery(cable, buffer[1] & 0x7F);
        return;
    }

    if (command == WEB_CMD_GET_SETTINGS) {
        if (length == 2) web_send_settings(cable);
        return;
    }

    if (command == WEB_CMD_SET_ROTATION) {
        if (length == 3 && buffer[1] <= ROTATION_270) {
            uint8_t value = buffer[1];
            rotation_set(value);
            eeprom_update(EE_ROTATION, value);
            eeprom_update(EE_ROTATION_MAGIC, 0xA5);
            web_send_settings(cable);
        }
        return;
    }

    if (command == WEB_CMD_SET_VELOCITY) {
        if (length == 3 && buffer[1] >= 1 && buffer[1] <= 127) {
            G_EE_MIDI_VELOCITY = buffer[1];
            eeprom_update(EE_MIDI_VELOCITY, G_EE_MIDI_VELOCITY);
            web_send_settings(cable);
        }
        return;
    }

    if (command == WEB_CMD_PALETTE_UPLOAD) {
        if (length == 131 && buffer[1] <= 2) {
            uint8_t component = buffer[1];
            for (uint16_t i = 0; i < 128; i++) {
                if (buffer[2 + i] > 0x3F) {
                    web_send_palette_ack(cable, component, WEB_STATUS_INVALID);
                    return;
                }
            }
            fastrgb_ch4_write_component6(component, &buffer[2]);
            eeprom_update(EE_CH4_MAGIC0, EE_CH4_MAGIC0_VALUE);
            eeprom_update(EE_CH4_MAGIC1, EE_CH4_MAGIC1_VALUE);
            web_send_palette_ack(cable, component, WEB_STATUS_OK);
        } else {
            web_send_palette_ack(cable, length > 1 ? buffer[1] : 0, WEB_STATUS_INVALID);
        }
        return;
    }

    if (command == WEB_CMD_PALETTE_DOWNLOAD) {
        if (length == 3 && buffer[1] <= 2) {
            web_send_palette_component(ctx, cable, buffer[1]);
        }
    }
}

// Apollo F0 5F ... F7 -> applies compressed RGB LED data, no SysEx reply.
// Apollo F0 6F ... F7 -> applies direct RGB LED data, no SysEx reply.
// Universal Identity F0 7E 7F 06 01 F7 -> returns the MF64 identity reply on the same MIDI cable.
// DJTT F0 00 01 79 01 ... F7 -> pushes configuration; config handler returns current configuration.
// DJTT F0 00 01 79 02 00 F7 -> returns current configuration.
// DJTT F0 00 01 79 03 01 F7 -> enters the DFU bootloader, no SysEx reply.
// DJTT F0 00 01 79 03 02 F7 -> factory reset and returns current configuration.
// DJTT F0 00 01 79 04 ... F7 -> handles bulk color transfer; read requests return bulk data chunks.
static void sysex_process(SysexContext* ctx, uint8_t cable)
{
    uint16_t length = ctx->length;
    uint8_t* buffer = ctx->buffer;
    midi_set_sysex_response_cable(cable);

    if (ctx->state == State_5F) {
        fastrgb_decompress(buffer, buffer + length - 1);
    }
    else if (ctx->state == State_6F) {
        fastrgb_list(buffer, buffer + length - 1);
    }
    else if (ctx->state == State_Web) {
        web_sysex_handle(ctx, cable, length);
    }
    else if (ctx->state == State_DJTT && length > 0) {
        uint8_t command = buffer[0];
        if (command > 0 && command <= MAX_COMMAND && sysExCommandMap[command - 1] != 0) {
            sysExCommandMap[command - 1](length - 1, &(buffer[1]));
        }
    }
    else if (ctx->state == State_NonRealtime) {
        if (length >= 2 && buffer[0] == 0x06 && buffer[1] == 0x01) {
            uint8_t payload[] = {0xf0, 0x7e, 0x7f, 0x06, 0x02,
                                0x00, MANUFACTURER_ID >> 8, MANUFACTURER_ID & 0x7f,
                                DEVICE_FAMILY,
                                DEVICE_MODEL,
                                (uint8_t)(((uint16_t)DEVICE_VERSION_YEAR) >> 8),
                                DEVICE_VERSION_YEAR & 0x7f,
                                DEVICE_VERSION_MONTH,
                                DEVICE_VERSION_DAY,
                                0xf7};
            midi_stream_sysex_cable(sizeof(payload), payload, cable);
        }
    }
}

void sysex_install_(uint8_t cmd, SysExFn fn)
{
    if (cmd > 0 && cmd <= MAX_COMMAND) {
        sysExCommandMap[cmd - 1] = fn;
    }
}

void sysex_handle_3sc(MIDI_EventPacket_t* packet)
{
    SysexContext* ctx = &sysex_ctx;
    uint8_t cable = sysex_packet_cable(packet);

    if (!ctx->reading) {
        ctx->reading = true;
        ctx->length = 0;
        sysex_owner_cable = cable;

        if (packet->Data1 == 0xf0 && packet->Data2 == 0x7e && packet->Data3 == 0x7f) {
            ctx->state = State_NonRealtime;
        }
        else if (packet->Data1 == 0xf0 && packet->Data2 == MIDI_MFR_ID_0 && packet->Data3 == MIDI_MFR_ID_1) {
            ctx->state = State_CheckMID;
        }
        else if (packet->Data1 == 0xf0 && packet->Data2 == 0x7D && packet->Data3 == 0x4D) {
            ctx->state = State_WebPrefix;
        }
        else if (packet->Data1 == 0xf0 && packet->Data2 == 0x6f) {
            ctx->state = State_6F;
            sysex_append(ctx, packet->Data3);
        }
        else if (packet->Data1 == 0xf0 && packet->Data2 == 0x5f) {
            ctx->state = State_5F;
            sysex_append(ctx, packet->Data3);
        }
        else {
            ctx->state = State_Invalid;
        }
        return;
    }

    if (!sysex_accept_packet(packet)) return;

    if (ctx->state == State_WebPrefix) {
        if (packet->Data1 == 0x46 && packet->Data2 == 0x36 && packet->Data3 == 0x34) {
            ctx->state = State_Web;
        } else {
            ctx->state = State_Invalid;
        }
        return;
    }

    if (ctx->state == State_CheckMID) {
        if (packet->Data1 == MIDI_MFR_ID_2) {
            ctx->state = State_DJTT;
            sysex_append(ctx, packet->Data2);
            sysex_append(ctx, packet->Data3);
        } else {
            ctx->state = State_Invalid;
        }
        return;
    }

    if (ctx->state == State_Invalid) return;

    sysex_append(ctx, packet->Data1);
    sysex_append(ctx, packet->Data2);
    sysex_append(ctx, packet->Data3);
}

void sysex_handle_3e(MIDI_EventPacket_t* packet)
{
    SysexContext* ctx = &sysex_ctx;
    uint8_t cable = sysex_packet_cable(packet);

    if (ctx->reading && cable != sysex_owner_cable) return;
    if (ctx->reading) {
        cable = sysex_owner_cable;
        ctx->reading = false;

        if (ctx->state == State_CheckMID) {
            if (packet->Data1 == MIDI_MFR_ID_2) {
                ctx->state = State_DJTT;
                sysex_append(ctx, packet->Data2);
                sysex_append(ctx, packet->Data3);
                if (ctx->state != State_Invalid) sysex_process(ctx, cable);
            }
        }
        else if (ctx->state != State_Invalid) {
            sysex_append(ctx, packet->Data1);
            sysex_append(ctx, packet->Data2);
            sysex_append(ctx, packet->Data3);
            if (ctx->state != State_Invalid) sysex_process(ctx, cable);
        }
    }
    else if (packet->Data1 == 0xf0 && packet->Data2 == 0x6e) {
        // Apollo F0 6E F7 -> clears all LED state, no SysEx reply.
        fastrgb_clear();
    }

    sysex_reset(ctx);
}

void sysex_handle_2e(MIDI_EventPacket_t* packet)
{
    SysexContext* ctx = &sysex_ctx;
    uint8_t cable = sysex_packet_cable(packet);

    if (ctx->reading && cable != sysex_owner_cable) return;
    if (ctx->reading) {
        cable = sysex_owner_cable;
        ctx->reading = false;
        if (ctx->state != State_Invalid) {
            sysex_append(ctx, packet->Data1);
            sysex_append(ctx, packet->Data2);
            if (ctx->state != State_Invalid) sysex_process(ctx, cable);
        }
    }

    sysex_reset(ctx);
}

void sysex_handle_1e(MIDI_EventPacket_t* packet)
{
    SysexContext* ctx = &sysex_ctx;
    uint8_t cable = sysex_packet_cable(packet);

    if (ctx->reading && cable != sysex_owner_cable) return;
    if (ctx->reading) {
        cable = sysex_owner_cable;
        ctx->reading = false;
        if (ctx->state != State_Invalid) {
            sysex_append(ctx, packet->Data1);
            if (ctx->state != State_Invalid) sysex_process(ctx, cable);
        }
    }

    sysex_reset(ctx);
}
