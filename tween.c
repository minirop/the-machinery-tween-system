static struct tm_logger_api* tm_logger_api;
static struct tm_entity_api* tm_entity_api;
static struct tm_tween_api* tm_tween_api;
static struct tm_graph_interpreter_api* tm_graph_interpreter_api;
static struct tm_properties_view_api* tm_properties_view_api;
static struct tm_the_truth_api* tm_the_truth_api;
static struct tm_localizer_api *tm_localizer_api;
static struct tm_allocator_api *tm_allocator_api;

#include "tween.h"

#include <foundation/api_registry.h>
#include <foundation/the_truth.h>
#include <foundation/the_truth_types.h>
#include <foundation/macros.h>
#include <foundation/log.h>
#include <foundation/localizer.h>
#include <foundation/allocator.h>
#include <foundation/carray.inl>

#include <plugins/entity/entity.h>
#include <plugins/editor_views/properties.h>
#include <plugins/editor_views/graph.h>
#include <plugins/graph_interpreter/graph_component.h>
#include <plugins/graph_interpreter/graph_component_node_type.h>
#include <plugins/graph_interpreter/graph_interpreter.h>

#include <string.h>

#include "easing.inl"

typedef uint64_t tm_string_hash_t;

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
    float from;
    float to;
    float duration;
    float elapsed;
    easingFunction easing;

    uint32_t id;

    bool paused;
};

static uint32_t next_id = 1; // id = 0 means error/no tween

struct tm_tween_manager_o
{
    tm_entity_context_o *ctx;
    tm_tween_item_o *tweens;
};

static tm_tween_item_o * find_tween_item(uint32_t id)
{
    tm_tween_manager_o * manager = tm_tween_api->manager;
    uint32_t n_tweens = (uint32_t)tm_carray_size(manager->tweens);

    for (uint32_t i = 0; i < n_tweens; ++i)
    {
        tm_tween_item_o * item = &manager->tweens[i];
        if (item->id == id)
        {
            return item;
        }
    }

    return NULL;
}

static void tween_init(struct tm_entity_context_o *ctx, tm_entity_system_o *inst, struct tm_entity_commands_o *commands)
{

}

static void tween_update(struct tm_entity_context_o *ctx, tm_entity_system_o *inst, struct tm_entity_commands_o *commands)
{
    const double dt = tm_entity_api->get_blackboard_double(ctx, TM_ENTITY_BB__DELTA_TIME, 1.0 / 60.0);
    const double editor = tm_entity_api->get_blackboard_double(ctx, TM_ENTITY_BB__EDITOR, 0.0);
    if (editor) return;

    tm_tween_manager_o *manager = (tm_tween_manager_o *)inst;
    uint32_t n_tweens = (uint32_t)tm_carray_size(manager->tweens);
    uint32_t n_finished_tweens = 0;
    for (uint32_t i = 0; i < n_tweens; ++i)
    {
        tm_tween_item_o * item = &manager->tweens[i];

        if (item->elapsed >= item->duration)
        {
            n_finished_tweens++;
        }
    }

    if (n_finished_tweens)
    {
        uint32_t last_position = n_tweens - 1;

        uint32_t i = 0;
        while (i < last_position)
        {
            tm_tween_item_o item = manager->tweens[i];

            if (item.elapsed >= item.duration)
            {
                manager->tweens[i] = manager->tweens[last_position];
                manager->tweens[last_position] = item;
                --last_position;
            }
            else
            {
                ++i;
            }
        }

        tm_carray_shrink(manager->tweens, i);
    }

    for (uint32_t i = 0; i < n_tweens; ++i)
    {
        tm_tween_item_o * item = &manager->tweens[i];

        if (!item->paused)
        {
            item->elapsed += dt;
        }
    }


}

static void tween_shutdown(struct tm_entity_context_o *ctx, tm_entity_system_o *inst, struct tm_entity_commands_o *commands)
{

}

static void register_tween_system(struct tm_entity_context_o *ctx)
{
    tm_allocator_i a;
    tm_entity_api->create_child_allocator(ctx, TM_TT_TYPE__GRAPH_COMPONENT, &a);

    tm_tween_manager_o * manager = tm_alloc(&a, sizeof(*manager));
    *(manager) = (tm_tween_manager_o){
        .ctx = ctx,
        .tweens = NULL,
    };

    const tm_entity_system_i tween_system = {
        .ui_name = TM_TWEEN_SYSTEM,
        .hash = TM_TWEEN_SYSTEM_HASH,
        .init = tween_init,
        .update = tween_update,
        .shutdown = tween_shutdown,
        .inst = (tm_entity_system_o *)manager,
    };
    tm_entity_api->register_system(ctx, &tween_system);
}

static inline void get_tween_variable(tm_graph_interpreter_context_t *ctx, tm_string_hash_t name, uint32_t *value)
{
    tm_graph_interpreter_wire_content_t var_w = tm_graph_interpreter_api->read_variable(ctx->interpreter, name);
    if (var_w.n)
        *value = *(uint32_t *)var_w.data;
}

static inline void set_tween_variable(tm_graph_interpreter_context_t *ctx, tm_string_hash_t name, uint32_t value)
{
    uint32_t *var = tm_graph_interpreter_api->write_variable(ctx->interpreter, name, 1, sizeof(*var));
    *var = value;
}

// NODES
//----------------------------------------------------
enum {
    TWEEN_CREATE__IN_WIRE,
    TWEEN_CREATE__FROM,
    TWEEN_CREATE__TO,
    TWEEN_CREATE__DURATION,
    TWEEN_CREATE__EASING,
    TWEEN_CREATE__OUT_WIRE,
    TWEEN_CREATE__OUT_TWEEN,
};

static const tm_graph_generic_value_t tween_from_default_value = { .f = (float[1]){ 0 } };
static const tm_graph_generic_value_t tween_to_default_value = { .f = (float[1]){ 1 } };
static const tm_graph_generic_value_t tween_duration_default_value = { .f = (float[1]){ 1 } };

static void tween_create_f(tm_graph_interpreter_context_t *ctx)
{
    const tm_graph_interpreter_wire_content_t from_w = tm_graph_interpreter_api->read_wire(ctx->interpreter, ctx->wires[TWEEN_CREATE__FROM]);
    const tm_graph_interpreter_wire_content_t to_w = tm_graph_interpreter_api->read_wire(ctx->interpreter, ctx->wires[TWEEN_CREATE__TO]);
    const tm_graph_interpreter_wire_content_t duration_w = tm_graph_interpreter_api->read_wire(ctx->interpreter, ctx->wires[TWEEN_CREATE__DURATION]);
    const tm_graph_interpreter_wire_content_t easing_w = tm_graph_interpreter_api->read_wire(ctx->interpreter, ctx->wires[TWEEN_CREATE__EASING]);

    const float from = from_w.n > 0 ? *(float *)from_w.data : *tween_from_default_value.f;
    const float to = to_w.n > 0 ? *(float *)to_w.data : *tween_to_default_value.f;
    const float duration = duration_w.n > 0 ? *(float *)duration_w.data : *tween_duration_default_value.f;
    const uint32_t easing = easing_w.n > 0 ? *(uint32_t *)easing_w.data : TM_TWEEN_EASING_ITEM_LINEAR;

    tm_tween_item_o *tween = tm_tween_api->create(from, to, duration, easingFunctions[easing]);
    uint32_t *v = tm_graph_interpreter_api->write_wire(ctx->interpreter, ctx->wires[TWEEN_CREATE__OUT_TWEEN], 1, sizeof(uint32_t));
    *v = tween->id;

    tm_graph_interpreter_api->trigger_wire(ctx->interpreter, ctx->wires[TWEEN_CREATE__OUT_WIRE]);
}

static tm_graph_component_node_type_i tween_create_node = {
    .definition_path = __FILE__,
    .name = "tm_tween_create",
    .category = TM_LOCALIZE_LATER("Tween"),
    .static_connectors.in = {
        { "", TM_TT_TYPE_HASH__GRAPH_EVENT },
        { "from", TM_TT_TYPE_HASH__FLOAT, .optional = true, .default_value = &tween_from_default_value },
        { "to", TM_TT_TYPE_HASH__FLOAT, .optional = true, .default_value = &tween_to_default_value },
        { "duration", TM_TT_TYPE_HASH__FLOAT, .optional = true, .default_value = &tween_duration_default_value },
        { "easing", TM_TT_TYPE_HASH__UINT32_T, TM_TT_TYPE_HASH__EASING_ITEM },
    },
    .static_connectors.num_in = 5,
    .static_connectors.out = {
        { "", TM_TT_TYPE_HASH__GRAPH_EVENT },
        { "tween", TM_TT_TYPE_HASH__TWEEN_ITEM },
    },
    .static_connectors.num_out = 2,
    .run = tween_create_f,
};

//----------------------------------------------------
enum {
    TWEEN_DESTROY__IN_WIRE,
    TWEEN_DESTROY__TWEEN,
    TWEEN_DESTROY__OUT_WIRE,
};

static void tween_destroy_f(tm_graph_interpreter_context_t *ctx)
{
    const tm_graph_interpreter_wire_content_t tween_w = tm_graph_interpreter_api->read_wire(ctx->interpreter, ctx->wires[TWEEN_DESTROY__TWEEN]);

    if (tween_w.n == 0)
        return;

    uint32_t tween_id = *(uint32_t *)tween_w.data;

    tm_tween_item_o * item = find_tween_item(tween_id);
    if (item)
        tm_tween_api->destroy(item);

    tm_graph_interpreter_api->trigger_wire(ctx->interpreter, ctx->wires[TWEEN_DESTROY__OUT_WIRE]);
}

static tm_graph_component_node_type_i tween_destroy_node = {
    .definition_path = __FILE__,
    .name = "tm_tween_destroy",
    .category = TM_LOCALIZE_LATER("Tween"),
    .static_connectors.in = {
        { "", TM_TT_TYPE_HASH__GRAPH_EVENT },
        { "tween", TM_TT_TYPE_HASH__TWEEN_ITEM },
    },
    .static_connectors.num_in = 2,
    .static_connectors.out = {
        { "", TM_TT_TYPE_HASH__GRAPH_EVENT },
    },
    .static_connectors.num_out = 1,
    .run = tween_destroy_f,
};
//----------------------------------------------------
enum {
    TWEEN_IS_RUNNING__TWEEN,
    TWEEN_IS_RUNNING__OUT_IS_RUNNING,
};

static void tween_is_running_f(tm_graph_interpreter_context_t *ctx)
{
    const tm_graph_interpreter_wire_content_t tween_w = tm_graph_interpreter_api->read_wire(ctx->interpreter, ctx->wires[TWEEN_IS_RUNNING__TWEEN]);

    if (tween_w.n == 0)
        return;

    uint32_t tween_id = *(uint32_t *)tween_w.data;
    bool *is_running = tm_graph_interpreter_api->write_wire(ctx->interpreter, ctx->wires[TWEEN_IS_RUNNING__OUT_IS_RUNNING], 1, sizeof(*is_running));

    tm_tween_item_o * item = find_tween_item(tween_id);
    *is_running = item != NULL;
}

static tm_graph_component_node_type_i tween_is_running_node = {
    .definition_path = __FILE__,
    .name = "tm_tween_is_running",
    .category = TM_LOCALIZE_LATER("Tween"),
    .static_connectors.in = {
        { "tween", TM_TT_TYPE_HASH__TWEEN_ITEM },
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
    TWEEN_IS_PAUSED__TWEEN,
    TWEEN_IS_PAUSED__OUT_IS_PAUSED,
};

static void tween_is_paused_f(tm_graph_interpreter_context_t *ctx)
{
    const tm_graph_interpreter_wire_content_t tween_w = tm_graph_interpreter_api->read_wire(ctx->interpreter, ctx->wires[TWEEN_IS_PAUSED__TWEEN]);

    if (tween_w.n == 0)
        return;

    uint32_t tween_id = *(uint32_t *)tween_w.data;
    bool *is_paused = tm_graph_interpreter_api->write_wire(ctx->interpreter, ctx->wires[TWEEN_IS_PAUSED__OUT_IS_PAUSED], 1, sizeof(*is_paused));

    tm_tween_item_o * item = find_tween_item(tween_id);
    if (item)
    {
        *is_paused = item->paused;
    }
    else
    {
        *is_paused = false;
    }
}

static tm_graph_component_node_type_i tween_is_paused_node = {
    .definition_path = __FILE__,
    .name = "tm_tween_is_paused",
    .category = TM_LOCALIZE_LATER("Tween"),
    .static_connectors.in = {
        { "tween", TM_TT_TYPE_HASH__TWEEN_ITEM },
    },
    .static_connectors.num_in = 1,
    .static_connectors.out = {
        { "is paused", TM_TT_TYPE_HASH__BOOL },
    },
    .static_connectors.num_out = 1,
    .run = tween_is_paused_f,
};
//----------------------------------------------------
enum {
    PAUSE_TWEEN__IN_EVENT,
    PAUSE_TWEEN__TWEEN,
    PAUSE_TWEEN__PAUSE,
    PAUSE_TWEEN__OUT_EVENT,
};

static const tm_graph_generic_value_t tween_pause_default_value = { .boolean = (bool[1]){ false } };

static void tween_pause_f(tm_graph_interpreter_context_t *ctx)
{
    const tm_graph_interpreter_wire_content_t tween_w = tm_graph_interpreter_api->read_wire(ctx->interpreter, ctx->wires[PAUSE_TWEEN__TWEEN]);
    const tm_graph_interpreter_wire_content_t pause_w = tm_graph_interpreter_api->read_wire(ctx->interpreter, ctx->wires[PAUSE_TWEEN__PAUSE]);

    if (tween_w.n == 0)
        return;

    uint32_t tween_id = *(uint32_t *)tween_w.data;
    const bool pause = pause_w.n > 0 ? *(bool *)pause_w.data : *tween_pause_default_value.boolean;

    tm_tween_item_o * item = find_tween_item(tween_id);
    if (item)
    {
        item->paused = pause;
    }

    tm_graph_interpreter_api->trigger_wire(ctx->interpreter, ctx->wires[PAUSE_TWEEN__OUT_EVENT]);
}

static tm_graph_component_node_type_i pause_tween_node = {
    .definition_path = __FILE__,
    .name = "tm_pause_tween",
    .category = TM_LOCALIZE_LATER("Tween"),
    .static_connectors.in = {
        { "", TM_TT_TYPE_HASH__GRAPH_EVENT },
        { "tween", TM_TT_TYPE_HASH__TWEEN_ITEM },
        { "pause", TM_TT_TYPE_HASH__BOOL },
    },
    .static_connectors.num_in = 3,
    .static_connectors.out = {
        { "", TM_TT_TYPE_HASH__GRAPH_EVENT },
    },
    .static_connectors.num_out = 1,
    .run = tween_pause_f,
};
//----------------------------------------------------
enum {
    TWEEN_GET_FLOAT__TWEEN,
    TWEEN_GET_FLOAT__OUT_GET_FLOAT,
};

static void tween_get_float_f(tm_graph_interpreter_context_t *ctx)
{
    const tm_graph_interpreter_wire_content_t tween_w = tm_graph_interpreter_api->read_wire(ctx->interpreter, ctx->wires[TWEEN_GET_FLOAT__TWEEN]);

    if (tween_w.n == 0)
        return;

    uint32_t tween_id = *(uint32_t *)tween_w.data;

    float *float_value = tm_graph_interpreter_api->write_wire(ctx->interpreter, ctx->wires[TWEEN_GET_FLOAT__OUT_GET_FLOAT], 1, sizeof(*float_value));
    *float_value = 0;

    tm_tween_item_o * item = find_tween_item(tween_id);
    if (item)
    {
        if (item->elapsed < item->duration)
        {
            *float_value = item->from + (item->to - item->from) * item->easing(item->elapsed / item->duration);
        }
        else
        {
            *float_value = item->to;
        }
    }
}

static tm_graph_component_node_type_i tween_get_float_node = {
    .definition_path = __FILE__,
    .name = "tm_tween_get_float",
    .category = TM_LOCALIZE_LATER("Tween"),
    .static_connectors.in = {
        { "tween", TM_TT_TYPE_HASH__TWEEN_ITEM },
    },
    .static_connectors.num_in = 1,
    .static_connectors.out = {
        { "value", TM_TT_TYPE_HASH__FLOAT },
    },
    .static_connectors.num_out = 1,
    .run = tween_get_float_f,
};
//----------------------------------------------------
enum {
    GET_TWEEN_VARIABLE__NAME,
    GET_TWEEN_VARIABLE__OUT_VALUE,
};

static void get_tween_variable_node_f(tm_graph_interpreter_context_t *ctx)
{
    const tm_graph_interpreter_wire_content_t name_w = tm_graph_interpreter_api->read_wire(ctx->interpreter, ctx->wires[GET_TWEEN_VARIABLE__NAME]);

    if (name_w.n == 0)
        return;

    const tm_string_hash_t name = *(tm_string_hash_t *)name_w.data;

    uint32_t *value = tm_graph_interpreter_api->write_wire(ctx->interpreter, ctx->wires[GET_TWEEN_VARIABLE__OUT_VALUE], 1, sizeof(*value));

    get_tween_variable(ctx, name, value);
}

static tm_graph_component_node_type_i get_tween_variable_node = {
    .definition_path = __FILE__,
    .name = "tm_get_tween_variable",
    .category = TM_LOCALIZE_LATER("Graph Variables"),
    .static_connectors.in = {
        { "name", TM_TT_TYPE_HASH__STRING_HASH, TM_TT_TYPE_HASH__STRING },
    },
    .static_connectors.num_in = 1,
    .static_connectors.out = {
        { "value", TM_TT_TYPE_HASH__TWEEN_ITEM },
    },
    .static_connectors.num_out = 1,
    .run = get_tween_variable_node_f,
};
//----------------------------------------------------
enum {
    SET_TWEEN_VARIABLE__IN_EVENT,
    SET_TWEEN_VARIABLE__NAME,
    SET_TWEEN_VARIABLE__VALUE,
    SET_TWEEN_VARIABLE__OUT_EVENT,
};

static void set_tween_variable_node_f(tm_graph_interpreter_context_t *ctx)
{
    const tm_graph_interpreter_wire_content_t name_w = tm_graph_interpreter_api->read_wire(ctx->interpreter, ctx->wires[SET_TWEEN_VARIABLE__NAME]);
    const tm_graph_interpreter_wire_content_t value_w = tm_graph_interpreter_api->read_wire(ctx->interpreter, ctx->wires[SET_TWEEN_VARIABLE__VALUE]);

    if (name_w.n == 0)
        return;

    const tm_string_hash_t name = *(tm_string_hash_t *)name_w.data;
    uint32_t value = *(uint32_t *)value_w.data;

    set_tween_variable(ctx, name, value);

    tm_graph_interpreter_api->trigger_wire(ctx->interpreter, ctx->wires[SET_TWEEN_VARIABLE__OUT_EVENT]);
}

static tm_graph_component_node_type_i set_tween_variable_node = {
    .definition_path = __FILE__,
    .name = "tm_set_tween_variable",
    .category = TM_LOCALIZE_LATER("Graph Variables"),
    .static_connectors.in = {
        { "", TM_TT_TYPE_HASH__GRAPH_EVENT },
        { "name", TM_TT_TYPE_HASH__STRING_HASH, TM_TT_TYPE_HASH__STRING },
        { "value", TM_TT_TYPE_HASH__TWEEN_ITEM },
    },
    .static_connectors.num_in = 3,
    .static_connectors.out = {
        { "", TM_TT_TYPE_HASH__GRAPH_EVENT },
    },
    .static_connectors.num_out = 1,
    .run = set_tween_variable_node_f,
};
//----------------------------------------------------

static void load_nodes(struct tm_api_registry_api* reg, bool load)
{
    tm_graph_component_node_type_i* nodes[] = {
        &tween_create_node,
        &tween_get_float_node,
        &tween_is_running_node,
        &tween_is_paused_node,
        &pause_tween_node,
        &tween_destroy_node,
        &get_tween_variable_node,
        &set_tween_variable_node,
    };
    for (int i = 0; i != TM_ARRAY_COUNT(nodes); ++i)
    {
        tm_add_or_remove_implementation(reg, load, tm_graph_component_node_type_i, nodes[i]);
    }
}

static tm_tween_item_o* create(float from, float to, float duration, easingFunction easing)
{
    struct tm_tween_item_o item = {
        .from = from,
        .to = to,
        .duration = duration,
        .easing = easing,
        .id = next_id++,
        .paused = false,
    };
    tm_carray_push(tm_tween_api->manager->tweens, item, tm_allocator_api->system);
    return tm_carray_last(tm_tween_api->manager->tweens);
}

static void destroy(tm_tween_item_o* item)
{
    tm_tween_manager_o *manager = tm_tween_api->manager;
    uint32_t n_tweens = (uint32_t)tm_carray_size(manager->tweens);

    for (uint32_t i = 0; i < n_tweens; ++i)
    {
        if (manager->tweens[i].id == item->id)
        {
            if (i != n_tweens - 1)
            {
                tm_tween_item_o destroyed = manager->tweens[i];
                manager->tweens[i] = manager->tweens[n_tweens - 1];
                manager->tweens[n_tweens - 1] = destroyed;
            }

            tm_carray_shrink(manager->tweens, n_tweens - 1);
            break;
        }
    }
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
    tm_the_truth_api = tm_get_api(reg, tm_the_truth_api);
    tm_logger_api = tm_get_api(reg, tm_logger_api);
    tm_entity_api = tm_get_api(reg, tm_entity_api);
    tm_graph_interpreter_api = tm_get_api(reg, tm_graph_interpreter_api);
    tm_properties_view_api = tm_get_api(reg, tm_properties_view_api);
    tm_localizer_api = tm_get_api(reg, tm_localizer_api);
    tm_allocator_api = tm_get_api(reg, tm_allocator_api);
    tm_tween_api = tm_get_api(reg, tm_tween_api);

    tm_set_or_remove_api(reg, load, tm_tween_api, &api);

    tm_add_or_remove_implementation(reg, load, tm_the_truth_create_types_i, create_truth_types);
    tm_add_or_remove_implementation(reg, load, tm_graph_component_compile_data_i, compile_data_to_wire);
    tm_add_or_remove_implementation(reg, load, tm_entity_register_engines_simulation_i, register_tween_system);

    load_nodes(reg, load);
}
