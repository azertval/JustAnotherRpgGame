// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Logic/TileTaxonomy.h"

namespace hmi {

std::vector<TileCategory> tileTaxonomy() {
    using core::TileType;
    return {
        {.label = "Tile",
         .tiles = {{.type = TileType::Empty, .label = "Empty (eraser)"},
                   {.type = TileType::Solid, .label = "Solid"}},
         .subgroups = {}},
        {.label = "Marker",
         .tiles = {{.type = TileType::Entry, .label = "Entry"}},
         .subgroups = {}},
        // Terrain du RPG (LOT-08). Trois categories plutot qu'une : ce qui se marche, ce qui
        // arrete, ce qui fait franchir -- c'est la question que se pose le level designer devant
        // sa palette, bien avant celle de la matiere representee.
        {.label = "Ground",
         .tiles = {{.type = TileType::Grass, .label = "Grass"},
                   {.type = TileType::Dirt, .label = "Dirt"},
                   {.type = TileType::Sand, .label = "Sand"},
                   {.type = TileType::Pavement, .label = "Pavement"},
                   {.type = TileType::Alley, .label = "Alley"},
                   {.type = TileType::Planks, .label = "Planks"},
                   {.type = TileType::Flagstone, .label = "Flagstone"},
                   {.type = TileType::Snow, .label = "Snow"},
                   {.type = TileType::Water, .label = "Water"},
                   {.type = TileType::DeepWater, .label = "Deep water"}},
         .subgroups = {}},
        // Ce qui se marche, mais en peinant (pas encore joue : traverse comme un sol).
        {.label = "Rough ground",
         .tiles = {{.type = TileType::Mud, .label = "Mud"},
                   {.type = TileType::Rubble, .label = "Rubble"},
                   {.type = TileType::Bush, .label = "Bush"}},
         .subgroups = {}},
        {.label = "Obstacle",
         .tiles = {{.type = TileType::Wall, .label = "Wall"},
                   {.type = TileType::Cliff, .label = "Cliff"},
                   {.type = TileType::Tree, .label = "Tree"},
                   {.type = TileType::Rock, .label = "Rock"},
                   {.type = TileType::Pit, .label = "Pit"},
                   {.type = TileType::Lava, .label = "Lava"}},
         .subgroups = {}},
        // Le bati et le mobilier de la ville : ce que les plans de principe montrent.
        {.label = "Building",
         .tiles = {{.type = TileType::Roof, .label = "Roof"},
                   {.type = TileType::Column, .label = "Column / statue"},
                   {.type = TileType::Tiers, .label = "Tiers"},
                   {.type = TileType::Fence, .label = "Fence"},
                   {.type = TileType::LowWall, .label = "Low wall (cover)"},
                   {.type = TileType::Stall, .label = "Stall / awning"},
                   {.type = TileType::Crate, .label = "Crates / furniture"}},
         .subgroups = {}},
        {.label = "Crossing",
         .tiles = {{.type = TileType::Bridge, .label = "Bridge"},
                   {.type = TileType::Stairs, .label = "Stairs"},
                   {.type = TileType::Door, .label = "Door"}},
         .subgroups = {}},
    };
}

}  // namespace hmi
