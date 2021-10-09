#pragma once

typedef struct tm_tween_manager_o tm_tween_manager_o;
typedef struct tm_tween_item_o tm_tween_item_o;

typedef double (*easingFunction)(double);

struct tm_tween_api
{
	tm_tween_item_o* (*create)(float from, float to, float duration, easingFunction easing);
	void (*destroy)(tm_tween_item_o* item);
};

#define tm_tween_api_version TM_VERSION(1, 0, 0)

#define TM_TT_TYPE__TWEEN_ITEM "tm_tween_item"
#define TM_TT_TYPE_HASH__TWEEN_ITEM TM_STATIC_HASH("tm_tween_item", 0x13a429408501296aULL)

#define TM_TT_TYPE__EASING_ITEM "tm_easing_item"
#define TM_TT_TYPE_HASH__EASING_ITEM TM_STATIC_HASH("tm_easing_item", 0xea6caf6c94635110ULL)

#define TM_TWEEN_SYSTEM "tm_tween_system"
#define TM_TWEEN_SYSTEM_HASH TM_STATIC_HASH("tm_tween_system", 0xf3dd3e4ba4a2a5d5ULL)

enum tm_tween_easing_item {
    TM_TWEEN_EASING_ITEM_LINEAR,
    TM_TWEEN_EASING_ITEM_INSINE,
    TM_TWEEN_EASING_ITEM_OUTSINE,
    TM_TWEEN_EASING_ITEM_INOUTSINE,
    TM_TWEEN_EASING_ITEM_INQUAD,
    TM_TWEEN_EASING_ITEM_OUTQUAD,
    TM_TWEEN_EASING_ITEM_INOUTQUAD,
    TM_TWEEN_EASING_ITEM_INCUBIC,
    TM_TWEEN_EASING_ITEM_OUTCUBIC,
    TM_TWEEN_EASING_ITEM_INOUTCUBIC,
    TM_TWEEN_EASING_ITEM_INQUART,
    TM_TWEEN_EASING_ITEM_OUTQUART,
    TM_TWEEN_EASING_ITEM_INOUTQUART,
    TM_TWEEN_EASING_ITEM_INQUINT,
    TM_TWEEN_EASING_ITEM_OUTQUINT,
    TM_TWEEN_EASING_ITEM_INOUTQUINT,
    TM_TWEEN_EASING_ITEM_INEXPO,
    TM_TWEEN_EASING_ITEM_OUTEXPO,
    TM_TWEEN_EASING_ITEM_INOUTEXPO,
    TM_TWEEN_EASING_ITEM_INCIRC,
    TM_TWEEN_EASING_ITEM_OUTCIRC,
    TM_TWEEN_EASING_ITEM_INOUTCIRC,
    TM_TWEEN_EASING_ITEM_INBACK,
    TM_TWEEN_EASING_ITEM_OUTBACK,
    TM_TWEEN_EASING_ITEM_INOUTBACK,
    TM_TWEEN_EASING_ITEM_INELASTIC,
    TM_TWEEN_EASING_ITEM_OUTELASTIC,
    TM_TWEEN_EASING_ITEM_INOUTELASTIC,
    TM_TWEEN_EASING_ITEM_INBOUNCE,
    TM_TWEEN_EASING_ITEM_OUTBOUNCE,
    TM_TWEEN_EASING_ITEM_INOUTBOUNCE,
};
