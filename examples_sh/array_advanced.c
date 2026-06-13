#include <stdio.h>

#define LOR_IMPLEMENTATION
#define LOR_LEAKCHECK
#define LOR_STRIP_PREFIX
#include "../lor.h"

// A simple vector structure
typedef struct Vector3 {
    float x, y, z;
} Vector3;

// A custom structure that includes a nested dynamic array and a liblor StringView
typedef struct Entity {
    StringView name;
    Vector3 position;
    StringView *tags; // Nested dynamic array of StringView objects
} Entity;

int main(void) {
    Entity *entities = ARRAY_INIT;

    Entity player = {
        .name = sv_from_cstr("Player 1"),
        .position = {0.0f, 10.0f, 0.0f},
        .tags = ARRAY_INIT
    };
    StringView hero_tag = sv_from_cstr("hero");
    StringView mc_tag = sv_from_cstr("main_character");
    array_push(player.tags, hero_tag);
    array_push(player.tags, mc_tag);
    
    // Using array_push with a local struct variable
    array_push(entities, player);

    StringView *enemy_tags = ARRAY_INIT;
    StringView villain_tag = sv_from_cstr("villain");
    StringView boss_tag = sv_from_cstr("boss");
    array_push(enemy_tags, villain_tag);
    array_push(enemy_tags, boss_tag);

    // Using array_push_as for inline compound literal initialization
    array_push_as(entities, Entity,
        .name = sv_from_cstr("Dark Lord"),
        .position = {100.0f, 0.0f, 50.0f},
        .tags = enemy_tags
    );

    printf("--- Entities ---\n");
    for (size_t i = 0; i < array_size(entities); ++i) {
        printf("Entity: %.*s\n", (int)entities[i].name.size, entities[i].name.data);
        printf("  Position: (%.1f, %.1f, %.1f)\n", 
               entities[i].position.x, 
               entities[i].position.y, 
               entities[i].position.z);
        
        printf("  Tags: ");
        for (size_t t = 0; t < array_size(entities[i].tags); ++t) {
            StringView tag = entities[i].tags[t];
            printf("[%.*s] ", (int)tag.size, tag.data);
        }
        printf("\n\n");
    }

    for (size_t i = 0; i < array_size(entities); ++i) {
        array_deinit(&entities[i].tags);
    }
    array_deinit(&entities);

    printf("Memory leaks: %d\n", (int)leakcheck_report(stdout));
    return 0;
}
