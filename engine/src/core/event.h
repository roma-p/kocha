#pragma once
#include "defines.h"

typedef struct event_context {

    // 128 bytes
    union {
	i64 i64[2];
	u64 u64[2];
	f64 f64[2];

	i32 i34[4];
	u32 u34[4];
	f32 f34[4];

	i16 i16[8];
	u16 u16[8];

	i8 i8[16];
	u8 u8[16];

	char c[16];
    } data;

} event_context;

// shoudl return TRUE if handled.
typedef b8 (*PFN_on_event)(u16 code, void* sender, void* listener_inst, event_context data);

b8 event_initialize();
void event_shutdown();

KAPI b8 event_register(u16 code, void* listener, PFN_on_event on_event);
KAPI b8 event_unregister(u16 code, void* listener, PFN_on_event on_event);
KAPI b8 event_fire(u16 code, void* sender, event_context on_event);

// engines uses codes uptil 255. Application shall use beyound 255.
typedef enum system_event_code {
    EVENT_CODE_APPLICATION_QUIT = 0x01,
    EVENT_CODE_KEY_PRESSED      = 0x02,
    EVENT_CODE_KEY_RELEASED     = 0x03,
    EVENT_CODE_BUTTON_PRESSED   = 0x04,
    EVENT_CODE_BUTTON_RELEASED  = 0x05,
    EVENT_CODE_MOUSE_MOVED      = 0x06,
    EVENT_CODE_MOUSE_WHEEL      = 0x07,
    EVENT_CODE_RESIZED    		= 0x08,
    MAX_EVENT_CODE              = 0xFF,
}system_event_code;
