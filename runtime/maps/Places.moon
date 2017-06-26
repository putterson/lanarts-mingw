import MapRegion, combine_map_regions, map_regions_bbox from require "maps.MapRegion"

World = require "core.World"
{:MapCompiler} = require "maps.MapCompiler"
MapUtils = require "maps.MapUtils"
TileSets = require "tiles.Tilesets"
GenerateUtils = require "maps.GenerateUtils"
SourceMap = require "core.SourceMap"
Map = require "core.Map"
Region1 = require "maps.Region1"
OldMaps = require "maps.OldMaps"
ItemUtils = require "maps.ItemUtils"
ItemGroups = require "maps.ItemGroups"

import ConnectedRegions, FilledRegion
    from require "maps.MapElements"

cave = (n_outer = 3) -> ConnectedRegions {
    regions: {
        for i=1,5
            FilledRegion {
                shape: 'deformed_ellipse'
                size: {5, 10}
            }
        for i=1,n_outer
            FilledRegion {
                shape: 'deformed_ellipse'
                size: {7, 12}
            }
    }
    spread_scheme: 'box2d_solid_center'
    connection_scheme: 'direct'
}

-- The room before the dragon lair.
DragonLairFoyer = newtype {
    parent: MapCompiler
    root_node: ConnectedRegions {
        regions: for i=1,5 do ConnectedRegions {
            regions: {
                for i=1,1
                    FilledRegion {
                        shape: 'deformed_ellipse'
                        size: {5, 10}
                    }
                for i=1,1
                    FilledRegion {
                        shape: 'deformed_ellipse'
                        size: {7, 12}
                    }
                for i=1,1
                    FilledRegion {
                        shape: 'deformed_ellipse'
                        size: {12, 12}
                    }
            }
            spread_scheme: 'box2d_solid_center'
            connection_scheme: 'direct'
        }
        spread_scheme: 'box2d_solid_center'
        connection_scheme: 'direct'
    }
    tileset: TileSets.lair
    enemies: {
        "Fire Bat": 10
    }
    -- Called before compile() is called 
    generate: (cc) =>
        for enemy, amount in pairs @enemies
            for i=1,amount
                sqr = MapUtils.random_square(@map, nil)
                MapUtils.spawn_enemy(@map, enemy, sqr)
        for i=1,5
            ItemUtils.item_object_generate(@map, ItemGroups.basic_items)
        for i=1,3
            sqr = @random_square_not_near_wall()
            @map\square_apply(sqr, {add: {SourceMap.FLAG_SOLID, SourceMap.FLAG_HAS_OBJECT}, remove: SourceMap.FLAG_SEETHROUGH})
            MapUtils.spawn_decoration(@map, OldMaps.statue, sqr, @rng\random(0,17))
}

DragonLair = newtype {
    parent: MapCompiler
    root_node: ConnectedRegions {
        regions: {
            cave()
            cave()
        }
        spread_scheme: 'box2d_solid_center'
        connection_scheme: 'direct_light'
    }
    tileset: TileSets.lair
    enemies: {
        "Purple Dragon": 1
        "Fire Bat": 8
    }
    -- Called before compile() is called 
    generate: (cc) =>
        for enemy, amount in pairs @enemies
            for i=1,amount
                sqr = MapUtils.random_square(@map, nil)
                MapUtils.spawn_enemy(@map, enemy, sqr)
        for i=1,3
            sqr = MapUtils.random_square(@map, nil)
            Region1.generate_store(@map, sqr)
        for i=1,10
            ItemUtils.item_object_generate(@map, ItemGroups.basic_items)
        for i=1,9
            sqr = @random_square_not_near_wall()
            @map\square_apply(sqr, {add: {SourceMap.FLAG_SOLID, SourceMap.FLAG_HAS_OBJECT}, remove: SourceMap.FLAG_SEETHROUGH})
            MapUtils.spawn_decoration(@map, OldMaps.statue, sqr, @rng\random(0,17))
        for i=1,1
            sqr = @random_square_not_near_wall()
            randarts = {}
            for j=1,#World.players *2
                 append randarts, ItemUtils.randart_generate(2)
            append randarts, {type: "Magentite Key"}
            MapUtils.spawn_chest(@map, sqr, randarts)
        for i=1,3
            sqr = @random_square_not_near_wall()
            MapUtils.spawn_healing_square(@map, sqr)
        for i=1,2
            sqr = @random_square_not_near_wall()
            items = for i=1,2
                {type: "Gold", amount: @rng\random(2,5)}
            MapUtils.spawn_chest(@map, sqr, items)
}

Arena = newtype {
    parent: MapCompiler
    root_node: cave()
    tileset: TileSets.lair
    -- Called before compile() is called 
    generate: (cc) =>
        enemies = loadstring(os.getenv "ARENA_ENEMIES")()
        for enemy, amount in pairs enemies
            for i=1,amount
                sqr = MapUtils.random_square(@map, nil)
                MapUtils.spawn_enemy(@map, enemy, sqr)
}

return nilprotect {:DragonLair, :DragonLairFoyer, :Arena}
