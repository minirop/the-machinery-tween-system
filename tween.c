static struct tm_logger_api* tm_logger_api;
static struct tm_entity_api* tm_entity_api;
static struct tm_tween_api* tm_tween_api;
static struct tm_graph_interpreter_api* tm_graph_interpreter_api;
static struct tm_properties_view_api* tm_properties_view_api;
static struct tm_the_truth_api* tm_the_truth_api;
static struct tm_localizer_api *tm_localizer_api;

#include "tween.h"

#include <foundation/api_registry.h>
#include <foundation/the_truth.h>
#include <foundation/the_truth_types.h>
#include <foundation/macros.h>
#include <foundation/log.h>
#include <foundation/localizer.h>
#include <foundation/temp_allocator.h>

#include <plugins/entity/entity.h>
#include <plugins/editor_views/properties.h>
#include <plugins/editor_views/graph.h>
#include <plugins/graph_interpreter/graph_component.h>
#include <plugins/graph_interpreter/graph_component_node_type.h>
#include <plugins/graph_interpreter/graph_interpreter.h>

#include <string.h>

#include "easing.inl"

#define TWEEN_NAME_LENGTH 64
#define MAX_TWEEN_COUNT 16

static easingFunction easingFunctions[] = {
    [TM_TWEEN_EASING_ITEM_LINEAR]       = easeLinear,
    [TM_TWEEN_EASING_ITEM_INSINE]       = easeInSine,
    [TM_TWEEN_EASING_ITEM_OUTSINE]      = easeOutSine,
    [TM_TWEEN_EASING_ITEM_INOUTSINE]    = easeInOutSine,
    [TM_TWEEN_EASING_ITEM_INQUAD]       = easeInQuad,
    [TM_TWEEN_EASING_ITEM_OUTQUAD]      = easeOutQuad,
    [TM_TWEEN_EASING_ITEM_INOUTQUAD]    = easeInOutQuad,
    [TM_TWEEN_EASING_ITEM_INCUBIC]      = easeInCubic,
    [TM_TWEEN_EASING_ITEM_OUTCUBIC]     = easeOutCubic,
    [TM_TWEEN_EASING_ITEM_INOUTCUBIC]   = easeInOutCubic,
    [TM_TWEEN_EASING_ITEM_INQUART]      = easeInQuart,
    [TM_TWEEN_EASING_ITEM_OUTQUART]     = easeOutQuart,
    [TM_TWEEN_EASING_ITEM_INOUTQUART]   = easeInOutQuart,
    [TM_TWEEN_EASING_ITEM_INQUINT]      = easeInQuint,
    [TM_TWEEN_EASING_ITEM_OUTQUINT]     = easeOutQuint,
    [TM_TWEEN_EASING_ITEM_INOUTQUINT]   = easeInOutQuint,
    [TM_TWEEN_EASING_ITEM_INEXPO]       = easeInExpo,
    [TM_TWEEN_EASING_ITEM_OUTEXPO]      = easeOutExpo,
    [TM_TWEEN_EASING_ITEM_INOUTEXPO]    = easeInOutExpo,
    [TM_TWEEN_EASING_ITEM_INCIRC]       = easeInCirc,
    [TM_TWEEN_EASING_ITEM_OUTCIRC]      = easeOutCirc,
    [TM_TWEEN_EASING_ITEM_INOUTCIRC]    = easeInOutCirc,
    [TM_TWEEN_EASING_ITEM_INBACK]       = easeInBack,
    [TM_TWEEN_EASING_ITEM_OUTBACK]      = easeOutBack,
    [TM_TWEEN_EASING_ITEM_INOUTBACK]    = easeInOutBack,
    [TM_TWEEN_EASING_ITEM_INELASTIC]    = easeInElastic,
    [TM_TWEEN_EASING_ITEM_OUTELASTIC]   = easeOutElastic,
    [TM_TWEEN_EASING_ITEM_INOUTELASTIC] = easeInOutElastic,
    [TM_TWEEN_EASING_ITEM_INBOUNCE]     = easeInBounce,
    [TM_TWEEN_EASING_ITEM_OUTBOUNCE]    = easeOutBounce,
    [TM_TWEEN_EASING_ITEM_INOUTBOUNCE]  = easeInOutBounce,
};

// SYSTEM
struct tm_tween_item_o
{
    char name[TWEEN_NAME_LENGTH];
    float from;
    float to;
    float duration;
    float elapsed;
    easingFunction easing;

    bool paused;
};

struct tm_tween_o
{
    tm_tween_item_o items[MAX_TWEEN_COUNT];
    bool valid[MAX_TWEEN_COUNT];
};

static struct tm_tween_o DEFAULT_TWEEN_ENGINE = {
    { {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {} },
    { false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false }
};

static void tween_update(struct tm_entity_context_o *ctx, tm_entity_system_o *inst)
{
    const double dt = tm_entity_api->get_blackboard_double(ctx, TM_ENTITY_BB__DELTA_TIME, 1.0 / 60.0);
    const double editor = tm_entity_api->get_blackboard_double(ctx, TM_ENTITY_BB__EDITOR, 0.0);
    if (editor) return;

    for (int i = 0; i < MAX_TWEEN_COUNT; i++)
    {
        if (DEFAULT_TWEEN_ENGINE.valid[i])
        {
            DEFAULT_TWEEN_ENGINE.items[i].elapsed += dt;
            DEFAULT_TWEEN_ENGINE.valid[i] = DEFAULT_TWEEN_ENGINE.items[i].elapsed < DEFAULT_TWEEN_ENGINE.items[i].duration;
        }
    }
}

static void register_tween_system(struct tm_entity_context_o *ctx)
{
    for (int i = 0; i < MAX_TWEEN_COUNT; i++)
    {
        DEFAULT_TWEEN_ENGINE.valid[i] = false;
    }

    const tm_entity_system_i tween_system = {
        .ui_name = "tween_system",
        .hash = TM_STATIC_HASH("tween_system", 0xbbc44fedb0efd920ULL),
        .update = tween_update,
        .inst = (tm_entity_system_o *)ctx,
    };
    tm_entity_api->register_system(ctx, &tween_system);
}

// NODES
enum {
    TWEEN_CREATE__IN_WIRE,
    TWEEN_CREATE__NAME,
    TWEEN_CREATE__FROM,
    TWEEN_CREATE__TO,
    TWEEN_CREATE__DURATION,
    TWEEN_CREATE__EASING,
    TWEEN_CREATE__OUT_WIRE,
};

static void tween_create_f(tm_graph_interpreter_context_t *ctx)
{
    const tm_graph_interpreter_wire_content_t name_w = tm_graph_interpreter_api->read_wire(ctx->interpreter, ctx->wires[TWEEN_CREATE__NAME]);
    const tm_graph_interpreter_wire_content_t from_w = tm_graph_interpreter_api->read_wire(ctx->interpreter, ctx->wires[TWEEN_CREATE__FROM]);
    const tm_graph_interpreter_wire_content_t to_w = tm_graph_interpreter_api->read_wire(ctx->interpreter, ctx->wires[TWEEN_CREATE__TO]);
    const tm_graph_interpreter_wire_content_t duration_w = tm_graph_interpreter_api->read_wire(ctx->interpreter, ctx->wires[TWEEN_CREATE__DURATION]);
    const tm_graph_interpreter_wire_content_t easing_w = tm_graph_interpreter_api->read_wire(ctx->interpreter, ctx->wires[TWEEN_CREATE__EASING]);

    if (name_w.n == 0)
        return;

    const char *name = (const char *)name_w.data;
    const float from = from_w.n > 0 ? *(float *)from_w.data : 0;
    const float to = to_w.n > 0 ? *(float *)to_w.data : 0;
    const float duration = duration_w.n > 0 ? *(float *)duration_w.data : 1;
    const uint32_t easing = easing_w.n > 0 ? *(uint32_t *)easing_w.data : 0;

    tm_tween_api->create(name, from, to, duration, easingFunctions[easing]);
}

static tm_graph_component_node_type_i tween_create_node = {
    .definition_path = __FILE__,
    .name = "tm_tween_create",
    .display_name = "tm_tween_create",
    .category = TM_LOCALIZE_LATER("Tween"),
    .static_connectors.in = {
        { "", TM_TT_TYPE_HASH__GRAPH_EVENT },
        { "name", TM_TT_TYPE_HASH__STRING },
        { "from", TM_TT_TYPE_HASH__FLOAT },
        { "to", TM_TT_TYPE_HASH__FLOAT },
        { "duration", TM_TT_TYPE_HASH__FLOAT },
        { "easing", TM_TT_TYPE_HASH__UINT32_T, TM_TT_TYPE_HASH__EASING_ITEM },
    },
    .static_connectors.num_in = 6,
    .static_connectors.out = {
        { "", TM_TT_TYPE_HASH__GRAPH_EVENT },
    },
    .static_connectors.num_out = 1,
    .run = tween_create_f,
};

//----------------------------------------------------
enum {
    TWEEN_IS_RUNNING__NAME,
    TWEEN_IS_RUNNING__OUT_IS_RUNNING,
};

static void tween_is_running_f(tm_graph_interpreter_context_t *ctx)
{
    const tm_graph_interpreter_wire_content_t name_w = tm_graph_interpreter_api->read_wire(ctx->interpreter, ctx->wires[TWEEN_IS_RUNNING__NAME]);

    if (name_w.n == 0)
        return;

    const char *name = (const char *)name_w.data;
    bool *is_running = tm_graph_interpreter_api->write_wire(ctx->interpreter, ctx->wires[TWEEN_IS_RUNNING__OUT_IS_RUNNING], 1, sizeof(*is_running));
    *is_running = false;

    for (int i = 0; i < MAX_TWEEN_COUNT; i++)
    {
        if (DEFAULT_TWEEN_ENGINE.valid[i] == true && strncmp(DEFAULT_TWEEN_ENGINE.items[i].name, name, TWEEN_NAME_LENGTH) == 0)
        {
            *is_running = true;
            break;
        }
    }
}

static tm_graph_component_node_type_i tween_is_running_node = {
    .definition_path = __FILE__,
    .name = "tm_tween_is_running",
    .display_name = "tm_tween_is_running",
    .category = TM_LOCALIZE_LATER("Tween"),
    .static_connectors.in = {
        { "name", TM_TT_TYPE_HASH__STRING },
    },
    .static_connectors.num_in = 1,
    .static_connectors.out = {
        { "is running", TM_TT_TYPE_HASH__BOOL },
    },
    .static_connectors.num_out = 1,
    .run = tween_is_running_f,
};
//----------------------------------------------------
enum {
    TWEEN_GET_FLOAT__NAME,
    TWEEN_GET_FLOAT__OUT_GET_FLOAT,
};

static void tween_get_float_f(tm_graph_interpreter_context_t *ctx)
{
    const tm_graph_interpreter_wire_content_t name_w = tm_graph_interpreter_api->read_wire(ctx->interpreter, ctx->wires[TWEEN_GET_FLOAT__NAME]);

    if (name_w.n == 0)
        return;

    const char *name = (const char *)name_w.data;

    float *float_value = tm_graph_interpreter_api->write_wire(ctx->interpreter, ctx->wires[TWEEN_GET_FLOAT__OUT_GET_FLOAT], 1, sizeof(*float_value));
    *float_value = 0;

    for (int i = 0; i < MAX_TWEEN_COUNT; i++)
    {
        if (DEFAULT_TWEEN_ENGINE.valid[i] == true && strncmp(DEFAULT_TWEEN_ENGINE.items[i].name, name, TWEEN_NAME_LENGTH) == 0)
        {
            tm_tween_item_o * itm = &DEFAULT_TWEEN_ENGINE.items[i];
            *float_value = itm->from + (itm->to - itm->from) * itm->easing(itm->elapsed / itm->duration);
            break;
        }
    }
}

static tm_graph_component_node_type_i tween_get_float_node = {
    .definition_path = __FILE__,
    .name = "tm_tween_get_float",
    .display_name = "tm_tween_get_float",
    .category = TM_LOCALIZE_LATER("Tween"),
    .static_connectors.in = {
        { "name", TM_TT_TYPE_HASH__STRING },
    },
    .static_connectors.num_in = 1,
    .static_connectors.out = {
        { "value", TM_TT_TYPE_HASH__FLOAT },
    },
    .static_connectors.num_out = 1,
    .run = tween_get_float_f,
};

static void load_nodes(struct tm_api_registry_api* reg, bool load)
{
    tm_graph_component_node_type_i* nodes[] = {
        &tween_create_node,
        &tween_get_float_node,
        &tween_is_running_node,
    };
    for (int i = 0; i != TM_ARRAY_COUNT(nodes); ++i)
    {
        tm_add_or_remove_implementation(reg, load, TM_GRAPH_COMPONENT_NODE_INTERFACE_NAME, nodes[i]);
    }
}

static tm_tween_item_o* create(const char* name, float from, float to, float duration, easingFunction easing)
{
    for (int i = 0; i < MAX_TWEEN_COUNT; i++)
    {
        if (DEFAULT_TWEEN_ENGINE.valid[i] == false)
        {
            strncpy(DEFAULT_TWEEN_ENGINE.items[i].name, name, TWEEN_NAME_LENGTH);
            DEFAULT_TWEEN_ENGINE.items[i].from = from;
            DEFAULT_TWEEN_ENGINE.items[i].to = to;
            DEFAULT_TWEEN_ENGINE.items[i].duration = duration;
            DEFAULT_TWEEN_ENGINE.items[i].elapsed = 0;
            DEFAULT_TWEEN_ENGINE.items[i].easing = easing;
            DEFAULT_TWEEN_ENGINE.valid[i] = true;

            return &DEFAULT_TWEEN_ENGINE.items[i];
        }
    }

    tm_logger_api->printf(TM_LOG_TYPE_ERROR, "Only %d tweens can be active at the same time\n", MAX_TWEEN_COUNT);

    return NULL;
}

static void destroy(tm_tween_item_o* item)
{
    // nothing
    tm_logger_api->printf(TM_LOG_TYPE_ERROR, "destroying tween system\n");
}

static struct tm_tween_api api = {
    .create = create,
    .destroy = destroy,
};

static const char *easing_item_names_array[] = {
    [TM_TWEEN_EASING_ITEM_LINEAR]       = "Linear",
    [TM_TWEEN_EASING_ITEM_INSINE]       = "In Sine",
    [TM_TWEEN_EASING_ITEM_OUTSINE]      = "Out Sine",
    [TM_TWEEN_EASING_ITEM_INOUTSINE]    = "InOut Sine",
    [TM_TWEEN_EASING_ITEM_INQUAD]       = "In Quad",
    [TM_TWEEN_EASING_ITEM_OUTQUAD]      = "Out Quad",
    [TM_TWEEN_EASING_ITEM_INOUTQUAD]    = "InOut Quad",
    [TM_TWEEN_EASING_ITEM_INCUBIC]      = "In Cubic",
    [TM_TWEEN_EASING_ITEM_OUTCUBIC]     = "Out Cubic",
    [TM_TWEEN_EASING_ITEM_INOUTCUBIC]   = "InOut Cubic",
    [TM_TWEEN_EASING_ITEM_INQUART]      = "In Quart",
    [TM_TWEEN_EASING_ITEM_OUTQUART]     = "Out Quart",
    [TM_TWEEN_EASING_ITEM_INOUTQUART]   = "InOut Quart",
    [TM_TWEEN_EASING_ITEM_INQUINT]      = "In Quint",
    [TM_TWEEN_EASING_ITEM_OUTQUINT]     = "Out Quint",
    [TM_TWEEN_EASING_ITEM_INOUTQUINT]   = "InOut Quint",
    [TM_TWEEN_EASING_ITEM_INEXPO]       = "In Expo",
    [TM_TWEEN_EASING_ITEM_OUTEXPO]      = "Out Expo",
    [TM_TWEEN_EASING_ITEM_INOUTEXPO]    = "InOut Expo",
    [TM_TWEEN_EASING_ITEM_INCIRC]       = "In Circ",
    [TM_TWEEN_EASING_ITEM_OUTCIRC]      = "Out Circ",
    [TM_TWEEN_EASING_ITEM_INOUTCIRC]    = "InOut Circ",
    [TM_TWEEN_EASING_ITEM_INBACK]       = "In Back",
    [TM_TWEEN_EASING_ITEM_OUTBACK]      = "Out Back",
    [TM_TWEEN_EASING_ITEM_INOUTBACK]    = "InOut Back",
    [TM_TWEEN_EASING_ITEM_INELASTIC]    = "In Elastic",
    [TM_TWEEN_EASING_ITEM_OUTELASTIC]   = "Out Elastic",
    [TM_TWEEN_EASING_ITEM_INOUTELASTIC] = "InOut Elastic",
    [TM_TWEEN_EASING_ITEM_INBOUNCE]     = "In Bounce",
    [TM_TWEEN_EASING_ITEM_OUTBOUNCE]    = "Out Bounce",
    [TM_TWEEN_EASING_ITEM_INOUTBOUNCE]  = "InOut Bounce",
};

static float ui_easing_object(struct tm_properties_ui_args_t *args, tm_rect_t item_rect, const char *name, const char *tooltip, tm_tt_id_t object, uint32_t indent)
{
    return tm_properties_view_api->ui_uint32_popup_picker(args, item_rect, name, tooltip, object, 0, easing_item_names_array, TM_ARRAY_COUNT(easing_item_names_array));
}

static tm_properties_aspect_i *easing_type_properties_aspect = &(tm_properties_aspect_i){
    .custom_subobject_ui = ui_easing_object
};

static void create_truth_types(struct tm_the_truth_o *tt)
{
    tm_the_truth_property_definition_t easing_item_properties[] = { { "easing", TM_THE_TRUTH_PROPERTY_TYPE_UINT32_T } };
    const tm_tt_type_t easing_type = tm_the_truth_api->create_object_type(tt, TM_TT_TYPE__EASING_ITEM, easing_item_properties, TM_ARRAY_COUNT(easing_item_properties));
    tm_the_truth_api->set_aspect(tt, easing_type, TM_TT_ASPECT__PROPERTIES, easing_type_properties_aspect);
}

static bool compile_data_to_wire(tm_graph_interpreter_o *gr, uint32_t wire, const tm_the_truth_o *tt, tm_tt_id_t data_id, tm_strhash_t to_type_hash)
{
    const tm_tt_type_t type = tm_tt_type(data_id);
    const tm_strhash_t type_hash = tm_the_truth_api->type_name_hash(tt, type);
    const tm_the_truth_object_o *data_r = tm_tt_read(tt, data_id);


    if (TM_STRHASH_EQUAL(type_hash, TM_TT_TYPE_HASH__EASING_ITEM) && TM_STRHASH_EQUAL(to_type_hash, TM_TT_TYPE_HASH__UINT32_T)) {
        uint32_t *v = (uint32_t *)tm_graph_interpreter_api->write_wire(gr, wire, 1, sizeof(uint32_t));
        *v = tm_the_truth_api->get_uint32_t(tt, data_r, 0);
        return true;
    }

    return false;
}

TM_DLL_EXPORT void tm_load_plugin(struct tm_api_registry_api* reg, bool load)
{
    tm_the_truth_api = reg->get(TM_THE_TRUTH_API_NAME);
    tm_logger_api = reg->get(TM_LOGGER_API_NAME);
    tm_entity_api = reg->get(TM_ENTITY_API_NAME);
    tm_graph_interpreter_api = reg->get(TM_GRAPH_INTERPRETER_API_NAME);
    tm_properties_view_api = reg->get(TM_PROPERTIES_VIEW_API_NAME);
    tm_localizer_api = reg->get(TM_LOCALIZER_API_NAME);
    tm_tween_api = reg->get(TM_TWEEN_API_NAME);

    tm_set_or_remove_api(reg, load, TM_TWEEN_API_NAME, &api);

    tm_add_or_remove_implementation(reg, load, TM_THE_TRUTH_CREATE_TYPES_INTERFACE_NAME, create_truth_types);
    tm_add_or_remove_implementation(reg, load, TM_GRAPH_COMPONENT_COMPILE_DATA_INTERFACE_NAME, compile_data_to_wire);
    tm_add_or_remove_implementation(reg, load, TM_ENTITY_SIMULATION_REGISTER_ENGINES_INTERFACE_NAME, register_tween_system);

    load_nodes(reg, load);
}
