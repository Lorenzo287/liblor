// SPDX-License-Identifier: MIT

#include "lor/set.h"

#include <stdint.h>
#include <string.h>

typedef enum LorSetOperation {
    LOR_SET_OPERATION_UNION,
    LOR_SET_OPERATION_INTERSECTION,
    LOR_SET_OPERATION_DIFFERENCE,
    LOR_SET_OPERATION_SYMMETRIC_DIFFERENCE
} LorSetOperation;

static void *lor_set__read_ref(const void *set_ref) {
    void *set = NULL;
    if (set_ref != NULL) memcpy(&set, set_ref, sizeof(set));
    return set;
}

static void lor_set__write_ref(void *set_ref, void *set) {
    memcpy(set_ref, &set, sizeof(set));
}

LorSetConfig lor_set_config_bytes(void) {
    return lor_map_config_bytes();
}

LorSetConfig lor_set_config_string_view(LorSetKeyOwnership ownership) {
    return lor_map_config_string_view(ownership);
}

LorStatus lor_set_init_raw(void *set_ref, size_t element_size,
                           LorSetConfig config) {
    return lor_map_init_raw(set_ref, element_size, element_size, config);
}

size_t lor_set_size(const void *set) {
    return lor_map_size(set);
}

size_t lor_set_capacity(const void *set) {
    return lor_map_capacity(set);
}

LorStatus lor_set_reserve_raw(void *set_ref, size_t element_size,
                              size_t capacity) {
    return lor_map_reserve_raw(set_ref, element_size, element_size, capacity);
}

LorStatus lor_set_add_raw(void *set_ref, size_t element_size,
                          const void *key) {
    return lor_map_set_raw(set_ref, element_size, element_size, key);
}

int lor_set_contains_raw(const void *set, size_t element_size,
                         const void *key) {
    return lor_map_contains_raw(set, element_size, element_size, key);
}

int lor_set_remove_raw(void *set, size_t element_size, const void *key) {
    return lor_map_remove_raw(set, element_size, element_size, key);
}

void lor_set_clear(void *set) {
    lor_map_clear(set);
}

void lor_set_deinit(void *set_ref) {
    lor_map_deinit(set_ref);
}

static int lor_set__layout_valid(const void *set, size_t element_size) {
    return set == NULL ||
           (element_size != 0 &&
            lor_map_entry_size(set) == element_size &&
            lor_map_key_size(set) == element_size);
}

static int lor_set__configs_compatible(LorSetConfig a, LorSetConfig b) {
    if (a.hash != b.hash || a.equal != b.equal ||
        a.key_valid != b.key_valid || a.context != b.context)
        return 0;

    return a.clone == NULL || b.clone == NULL ||
           (a.clone == b.clone && a.drop == b.drop);
}

static int lor_set__nonempty_configs_compatible(const void *a,
                                                const void *b) {
    return lor_set_size(a) == 0 || lor_set_size(b) == 0 ||
           lor_set__configs_compatible(lor_map_get_config(a),
                                       lor_map_get_config(b));
}

static LorSetConfig lor_set__result_config(const void *a, const void *b) {
    LorSetConfig a_config = lor_map_get_config(a);
    LorSetConfig b_config = lor_map_get_config(b);
    if (a != NULL && a_config.clone != NULL) return a_config;
    if (b != NULL && b_config.clone != NULL) return b_config;
    if (lor_set_size(a) != 0) return lor_map_get_config(a);
    if (lor_set_size(b) != 0) return lor_map_get_config(b);
    if (a != NULL) return a_config;
    if (b != NULL) return b_config;
    return lor_set_config_bytes();
}

static const void *lor_set__key(const void *set, size_t element_size,
                                size_t index) {
    return (const unsigned char *)set + index * element_size;
}

static LorStatus lor_set__reserve_for_operation(void *result_ref,
                                                size_t element_size,
                                                const void *a,
                                                const void *b,
                                                LorSetOperation operation) {
    size_t a_size = lor_set_size(a);
    size_t b_size = lor_set_size(b);
    size_t capacity = 0;

    switch (operation) {
    case LOR_SET_OPERATION_UNION:
    case LOR_SET_OPERATION_SYMMETRIC_DIFFERENCE:
        if (b_size > SIZE_MAX - a_size) return LOR_STATUS_OVERFLOW;
        capacity = a_size + b_size;
        break;
    case LOR_SET_OPERATION_INTERSECTION:
        capacity = a_size < b_size ? a_size : b_size;
        break;
    case LOR_SET_OPERATION_DIFFERENCE:
        capacity = a_size;
        break;
    }

    return capacity != 0
               ? lor_set_reserve_raw(result_ref, element_size, capacity)
               : LOR_STATUS_OK;
}

static LorStatus lor_set__add_all(void *result_ref, size_t element_size,
                                  const void *source) {
    for (size_t i = 0; i < lor_set_size(source); ++i) {
        LorStatus status = lor_set_add_raw(
            result_ref, element_size,
            lor_set__key(source, element_size, i));
        if (status != LOR_STATUS_OK) return status;
    }
    return LOR_STATUS_OK;
}

static LorStatus lor_set__operation(void *result_ref, size_t element_size,
                                    const void *a, const void *b,
                                    LorSetOperation operation) {
    if (result_ref == NULL || lor_set__read_ref(result_ref) != NULL ||
        element_size == 0 || !lor_set__layout_valid(a, element_size) ||
        !lor_set__layout_valid(b, element_size) ||
        !lor_set__nonempty_configs_compatible(a, b))
        return LOR_STATUS_INVALID_ARGUMENT;

    void *result = NULL;
    LorStatus status = lor_set_init_raw(
        &result, element_size, lor_set__result_config(a, b));
    if (status != LOR_STATUS_OK) return status;

    status = lor_set__reserve_for_operation(
        &result, element_size, a, b, operation);
    if (status != LOR_STATUS_OK) {
        lor_set_deinit(&result);
        return status;
    }

    if (operation == LOR_SET_OPERATION_UNION) {
        status = lor_set__add_all(&result, element_size, a);
        if (status == LOR_STATUS_OK)
            status = lor_set__add_all(&result, element_size, b);
    } else {
        for (size_t i = 0; i < lor_set_size(a); ++i) {
            const void *key = lor_set__key(a, element_size, i);
            int in_b = lor_set_contains_raw(b, element_size, key);
            int include =
                operation == LOR_SET_OPERATION_INTERSECTION ? in_b : !in_b;
            if (include) {
                status =
                    lor_set_add_raw(&result, element_size, key);
                if (status != LOR_STATUS_OK) break;
            }
        }

        if (status == LOR_STATUS_OK &&
            operation == LOR_SET_OPERATION_SYMMETRIC_DIFFERENCE) {
            for (size_t i = 0; i < lor_set_size(b); ++i) {
                const void *key = lor_set__key(b, element_size, i);
                if (!lor_set_contains_raw(a, element_size, key)) {
                    status =
                        lor_set_add_raw(&result, element_size, key);
                    if (status != LOR_STATUS_OK) break;
                }
            }
        }
    }

    if (status != LOR_STATUS_OK) {
        lor_set_deinit(&result);
        return status;
    }

    lor_set__write_ref(result_ref, result);
    return LOR_STATUS_OK;
}

LorStatus lor_set_union_raw(void *result_ref, size_t element_size,
                            const void *a, const void *b) {
    return lor_set__operation(result_ref, element_size, a, b,
                              LOR_SET_OPERATION_UNION);
}

LorStatus lor_set_intersection_raw(void *result_ref, size_t element_size,
                                   const void *a, const void *b) {
    return lor_set__operation(result_ref, element_size, a, b,
                              LOR_SET_OPERATION_INTERSECTION);
}

LorStatus lor_set_difference_raw(void *result_ref, size_t element_size,
                                 const void *a, const void *b) {
    return lor_set__operation(result_ref, element_size, a, b,
                              LOR_SET_OPERATION_DIFFERENCE);
}

LorStatus lor_set_symmetric_difference_raw(void *result_ref,
                                           size_t element_size,
                                           const void *a, const void *b) {
    return lor_set__operation(
        result_ref, element_size, a, b,
        LOR_SET_OPERATION_SYMMETRIC_DIFFERENCE);
}

static int lor_set__relations_valid(const void *a, const void *b,
                                    size_t element_size) {
    return element_size != 0 &&
           lor_set__layout_valid(a, element_size) &&
           lor_set__layout_valid(b, element_size) &&
           lor_set__nonempty_configs_compatible(a, b);
}

int lor_set_is_subset_raw(const void *a, const void *b,
                          size_t element_size) {
    if (!lor_set__relations_valid(a, b, element_size) ||
        lor_set_size(a) > lor_set_size(b))
        return 0;

    for (size_t i = 0; i < lor_set_size(a); ++i) {
        if (!lor_set_contains_raw(
                b, element_size,
                lor_set__key(a, element_size, i)))
            return 0;
    }
    return 1;
}

int lor_set_equal_raw(const void *a, const void *b, size_t element_size) {
    return lor_set_size(a) == lor_set_size(b) &&
           lor_set_is_subset_raw(a, b, element_size);
}

int lor_set_is_proper_subset_raw(const void *a, const void *b,
                                 size_t element_size) {
    return lor_set_size(a) < lor_set_size(b) &&
           lor_set_is_subset_raw(a, b, element_size);
}

int lor_set_is_superset_raw(const void *a, const void *b,
                            size_t element_size) {
    return lor_set_is_subset_raw(b, a, element_size);
}

int lor_set_is_proper_superset_raw(const void *a, const void *b,
                                   size_t element_size) {
    return lor_set_is_proper_subset_raw(b, a, element_size);
}

int lor_set_is_disjoint_raw(const void *a, const void *b,
                            size_t element_size) {
    if (!lor_set__relations_valid(a, b, element_size)) return 0;

    const void *small = lor_set_size(a) <= lor_set_size(b) ? a : b;
    const void *large = small == a ? b : a;
    for (size_t i = 0; i < lor_set_size(small); ++i) {
        if (lor_set_contains_raw(
                large, element_size,
                lor_set__key(small, element_size, i)))
            return 0;
    }
    return 1;
}
