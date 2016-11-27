----
-- Generates the game maps, starting with high-level details (places that will be in the game)
-- and then generating actual tiles.
----
import map_place_object, ellipse_points, 
    LEVEL_PADDING, Region, RVORegionPlacer, 
    random_rect_in_rect, random_ellipse_in_ellipse, 
    ring_region_delta_func, default_region_delta_func, spread_region_delta_func,
    random_region_add, subregion_minimum_spanning_tree, region_minimum_spanning_tree,
    Tile, tile_operator from require "maps.GenerateUtils"

TileSets = require "tiles.Tilesets"
MapUtils = require "maps.MapUtils"
ItemUtils = require "maps.ItemUtils"
ItemGroups = require "maps.ItemGroups"
import make_tunnel_oper, make_rectangle_criteria, make_rectangle_oper
    from MapUtils

MapSequence = require "maps.MapSequence"
Vaults = require "maps.Vaults"
World = require "core.World"
SourceMap = require "core.SourceMap"
Map = require "core.Map"
OldMaps = require "maps.OldMaps"
Region1 = require "maps.Region1"

-- Generation constants and data
{   :FLAG_ALTERNATE, :FLAG_INNER_PERIMETER, :FLAG_DOOR_CANDIDATE, 
    :FLAG_OVERWORLD, :FLAG_ROOM, :FLAG_NO_ENEMY_SPAWN, :FLAG_NO_ITEM_SPAWN
} = Vaults

create_overworld_scheme = (tileset) -> {
    floor1: Tile.create(tileset.floor, false, true, {FLAG_OVERWORLD})
    floor2: Tile.create(tileset.floor_alt, false, true, {FLAG_OVERWORLD}) 
    wall1: Tile.create(tileset.wall, true, true, {FLAG_OVERWORLD})
    wall2: Tile.create(tileset.wall_alt, true, false)
}

create_dungeon_scheme = (tileset) -> {
    floor1: Tile.create(tileset.floor, false, true, {}, {FLAG_OVERWORLD})
    floor2: Tile.create(tileset.floor_alt, false, true, {}, {FLAG_OVERWORLD}) 
    wall1: Tile.create(tileset.wall, true, false, {}, {FLAG_OVERWORLD})
    wall2: Tile.create(tileset.wall_alt, true, false, {}, {FLAG_OVERWORLD})
}

OVERWORLD_TILESET = create_overworld_scheme(TileSets.grass)

OVERWORLD_DIM_LESS, OVERWORLD_DIM_MORE = 160, 160
SHELL = 10
OVERWORLD_CONF = (rng) -> {
    map_label: "Plain Valley"
    is_overworld: true
    size: if rng\random(0,2) == 0 then {85, 65} else {85, 65} 
    number_regions: rng\random(35,40)
    floor1: OVERWORLD_TILESET.floor1 
    floor2: OVERWORLD_TILESET.floor2
    wall1: OVERWORLD_TILESET.wall1
    wall2: OVERWORLD_TILESET.wall2
    rect_room_num_range: {0,0}
    rect_room_size_range: {10,15}
    rvo_iterations: 150
    n_stairs_down: 0
    n_stairs_up: 0
    connect_line_width: () -> rng\random(2,6)
    region_delta_func: ring_region_delta_func
    room_radius: () ->
        r = 5
        bound = rng\random(1,10)
        for j=1,rng\random(0,bound) do r += rng\randomf(0, 1)
        return r
    -- Dungeon objects/features
    monster_weights: () -> {["Giant Rat"]: 0, ["Chicken"]: 0, ["Cloud Elemental"]: 1, ["Turtle"]: 8, ["Spriggan"]: 2}
    n_statues: 4
}

DUNGEON_CONF = (rng) -> 
    -- Brown layout or blue layout?
    tileset = TileSets.crystal
    C = create_dungeon_scheme(tileset)
    -- Rectangle-heavy or polygon-heavy?
    switch rng\random(3)
        when 0
            -- Few, big, rooms?
            C.number_regions = rng\random(5,10)
            C.room_radius = () ->
                r = 4
                for j=1,rng\random(0,10) do r += rng\randomf(0, 1)
                return r
            C.rect_room_num_range = {2,3}
            C.rect_room_size_range = {10,15}
        when 1
            -- Mix?
            C.number_regions = rng\random(5,20)
            C.room_radius = () ->
                r = 2
                for j=1,rng\random(0,10) do r += rng\randomf(0, 1)
                return r
            C.rect_room_num_range = {2,10}
            C.rect_room_size_range = {7,15}
        when 2
            -- Mostly rectangular rooms?
            C.number_regions = rng\random(2,7)
            C.room_radius = () ->
                r = 2
                for j=1,rng\random(0,10) do r += rng\randomf(0, 1)
                return r
            C.rect_room_num_range = {10,15}
            C.rect_room_size_range = {7,15}

    return table.merge C, {
        map_label: "A Dungeon"
        size: if rng\random(0,2) == 0 then {55, 45} else {45, 55} 
        rvo_iterations: 20
        n_stairs_down: 3
        n_stairs_up: 0
        connect_line_width: () -> 2 + (if rng\random(5) == 4 then 1 else 0)
        region_delta_func: default_region_delta_func
        -- Dungeon objects/features
        monster_weights: () -> {["Giant Rat"]: 8, ["Cloud Elemental"]: 1, ["Chicken"]: 1}
        n_statues: 4
    }


make_rooms_with_tunnels = (map, rng, conf, area) ->
    oper = SourceMap.random_placement_operator {
        size_range: conf.rect_room_size_range
        rng: rng, :area
        amount_of_placements_range: conf.rect_room_num_range
        create_subgroup: false
        child_operator: (map, subgroup, bounds) ->
            --Purposefully convoluted for test purposes
            queryfn = () ->
                query = make_rectangle_criteria()
                return query(map, subgroup, bounds)
            oper = make_rectangle_oper(conf.floor2.id, conf.wall2.id, conf.wall2.seethrough, queryfn)
            if oper(map, subgroup, bounds)
                append map.rectangle_rooms, bounds
                --place_instances(rng, map, bounds)
                return true
            return false
    }
 
    oper map, SourceMap.ROOT_GROUP, area 
    tunnel_oper = make_tunnel_oper(rng, conf.floor1.id, conf.wall1.id, conf.wall1.seethrough)

    tunnel_oper map, SourceMap.ROOT_GROUP, area--{1,1, map.size[1]-1,map.size[2]-1}
    return map

connect_edges = (map, rng, conf, area, edges) ->
    for {p1, p2} in *edges
        tile = conf.floor1
        flags = {}
        rad1,rad2 = math.max(p1.w, p1.h)/2, math.max(p2.w, p2.h)/2
        line_width = conf.connect_line_width()
        if line_width <= 2 and p1\ortho_dist(p2) > (rng\random(3,6)+rad1+rad2)
            append flags, SourceMap.FLAG_TUNNEL
        if p2.id%5 <= 3 
            tile = conf.floor2
            append flags, FLAG_ALTERNATE
        fapply = nil 
        if rng\random(4) < 2 
            fapply = p1.line_connect 
        else 
            fapply = p1.arc_connect
        fapply p1, {
            :map, :area, target: p2, :line_width
            operator: (tile_operator tile, {matches_none: FLAG_ALTERNATE, matches_all: SourceMap.FLAG_SOLID, add: flags})
        }

make_rect_points = (x1,y1,x2,y2) ->
    return {{x1, y2}, {x2, y2}, {x2, y1}, {x1, y1}}

generate_area = (map, rng, conf, outer, padding) ->
    size = conf.size
    R = RVORegionPlacer.create {outer.points}-- {make_rect_points outer.x, outer.y, outer.x+outer.w,outer.x}

    for i=1,conf.number_regions
        -- Make radius of the circle:
        r, n_points, angle = conf.room_radius(),rng\random(3,10) ,rng\randomf(0, math.pi)
        {x1, y1, x2, y2} = outer\bbox()
        r = random_region_add rng, r*2,r*2, n_points, conf.region_delta_func(map, rng, outer), angle, R, {x1 + padding, y1 + padding, x2 - padding, y2 - padding}, true
        if r then outer\add(r)

    R\steps(conf.rvo_iterations)

    for region in *R.regions
        tile = (if rng\random(4) ~= 1 then conf.floor1 else conf.floor2)
        region\apply {
            map: map, area: outer\bbox(), operator: (tile_operator tile, {add: FLAG_ROOM})
        }

    -- Connect all the closest region pairs:
    edges = region_minimum_spanning_tree(R.regions)
    add_edge_if_unique = (p1,p2) ->
        for {op1, op2} in *edges
            if op1 == p1 and op2 == p2 or op2 == p1 and op1 == p2
                return
        append edges, {p1, p2}

    -- Append all < threshold in distance
    for i=1,#R.regions
        for j=i+1,#R.regions do if rng\random(0,3) == 1
            p1, p2 = R.regions[i], R.regions[j]
            dist = math.sqrt( (p2.x-p1.x)^2+(p2.y-p1.y)^2)
            if dist < rng\random(5,15)
                add_edge_if_unique p1, p2
    connect_edges map, rng, conf, outer\bbox(), edges

generate_subareas = (map, rng, regions) ->
    conf = OVERWORLD_CONF(rng)
    -- Generate the polygonal rooms, connected with lines & arcs
    for region in *regions
        generate_area map, rng, region.conf, region, SHELL

    edges = subregion_minimum_spanning_tree(regions, () -> rng\random(12) + rng\random(12))
    connect_edges map, rng, conf, nil, edges

    -- Diagonal pairs are a bit ugly. We can see through them but not pass them. Just open them up.
    SourceMap.erode_diagonal_pairs {:map, :rng, selector: {matches_all: SourceMap.FLAG_SOLID}}

    -- Detect the perimeter, important for the winding-tunnel algorithm.
    SourceMap.perimeter_apply {:map,
        candidate_selector: {matches_all: SourceMap.FLAG_SOLID}, inner_selector: {matches_none: SourceMap.FLAG_SOLID}
        operator: {add: SourceMap.FLAG_PERIMETER}
    }

    for region in *regions
        SourceMap.perimeter_apply {:map,
            area: region\bbox()
            candidate_selector: {matches_all: SourceMap.FLAG_SOLID}, inner_selector: {matches_all: FLAG_ALTERNATE, matches_none: SourceMap.FLAG_SOLID}
            operator: tile_operator region.conf.wall2 
        }

        -- Generate the rectangular rooms, connected with winding tunnels
    for region in *regions
        make_rooms_with_tunnels map, rng, region.conf, region\bbox() 

    SourceMap.perimeter_apply {:map
        candidate_selector: {matches_none: {SourceMap.FLAG_SOLID}}, 
        inner_selector: {matches_all: {SourceMap.FLAG_PERIMETER, SourceMap.FLAG_SOLID}}
        operator: {add: FLAG_INNER_PERIMETER}
    }
    for region in *regions
        if region.conf.is_overworld
            for subregion in *region.subregions
                region\apply {:map
                    operator: {remove: SourceMap.FLAG_TUNNEL}
                }
    -- Make sure doors dont get created in the overworld components:
    --SourceMap.rectangle_apply {:map, fill_operator: {matches_all: FLAG_OVERWORLD, remove: SourceMap.FLAG_TUNNEL}}
    SourceMap.perimeter_apply {:map,
        candidate_selector: {matches_all: {SourceMap.FLAG_TUNNEL}, matches_none: {FLAG_ROOM, SourceMap.FLAG_SOLID}}, 
        inner_selector: {matches_all: {FLAG_ROOM}, matches_none: {FLAG_DOOR_CANDIDATE, SourceMap.FLAG_SOLID}}
        operator: {add: FLAG_DOOR_CANDIDATE}
    }

    filter_door_candidates = (x1,y1,x2,y2) ->
        SourceMap.rectangle_apply {:map
            fill_operator: {remove: FLAG_DOOR_CANDIDATE}, area: {x1, y1, x2, y2}
        }
    filter_random_third = (x1,y1,x2,y2) ->
        w,h = (x2 - x1), (y2 - y1)
        if rng\random(0,2) == 0 
            filter_door_candidates(x2 + w/3, y1-1, x2+1, y2+1)
        if rng\random(0,2) == 0 
            filter_door_candidates(x1-1, y1-1, x1 + w/3, y2+1)
        if rng\random(0,2) == 0 
            filter_door_candidates(x1-1, y1+h/3, x2+1, y2+1)
        if rng\random(0,2) == 0 
            filter_door_candidates(x1-1, y1-1, x2+1, y2 - h/3)
    for region in *regions
        -- Unbox the region:
        for {:x,:y,:w,:h} in *region.subregions
            if rng\random(3) == 0
                filter_random_third(x,y,x+w,y+h)
    for {x1,y1,x2,y2} in *map.rectangle_rooms
        -- Account for there already being a perimeter -- don't want to remove tunnels too far, get weird artifacts.
        filter_random_third(x1+1,y1+1,x2-1,y2-1)

generate_game_map = (map, place_object, place_monsters) ->
    M = Map.create {
        map: map
        label: map.map_label
        instances: map.instances
        wandering_enabled: map.wandering_enabled
    }
    return M

-- Returns a post-creation callback to be called on game_map

overworld_spawns = (map) ->
    gen_feature = (sprite, solid, seethrough = true) -> (px, py) -> 
        Feature.create M, {x: px*32+16, y: py*32+16, :sprite, :solid, :seethrough}

    for region in *map.regions
        area = region\bbox()
        conf = region.conf
        for xy in *SourceMap.rectangle_match {:map, selector: {matches_none: {SourceMap.FLAG_HAS_OBJECT, SourceMap.FLAG_SOLID}, matches_all: {FLAG_DOOR_CANDIDATE}}}
            MapUtils.spawn_door(map, xy)
        for i=1,conf.n_statues
            sqr = MapUtils.random_square(map, area, {matches_none: {FLAG_INNER_PERIMETER, SourceMap.FLAG_HAS_OBJECT, SourceMap.FLAG_SOLID}})
            if not sqr
                break
            map\square_apply(sqr, {add: SourceMap.FLAG_SOLID, remove: SourceMap.FLAG_SEETHROUGH})
            MapUtils.spawn_decoration(map, OldMaps.statue, sqr, random(0,17))
        -- for i=1,conf.n_shops
        --     sqr = MapUtils.random_square(map, area, {matches_none: {SourceMap.FLAG_HAS_OBJECT, SourceMap.FLAG_SOLID}})
        --     if not sqr
        --         break
        --     Region1.generate_store(map, sqr)
        OldMaps.generate_from_enemy_entries(map, OldMaps.medium_animals, 5, area, {matches_none: {SourceMap.FLAG_SOLID, FLAG_NO_ENEMY_SPAWN}})

overworld_features = (map) ->
    {mw, mh} = map.size
    place_feature = (template, region_filter) ->
       -- Function to try a single placement, returns success:
       attempt_placement = (template) ->
           orient = map.rng\random_choice {
               SourceMap.ORIENT_DEFAULT, SourceMap.ORIENT_FLIP_X, SourceMap.ORIENT_FLIP_Y,
               SourceMap.ORIENT_TURN_90, SourceMap.ORIENT_TURN_180, SourceMap.ORIENT_TURN_270
           }
           for r in *map.regions
               if not region_filter(r)
                   continue
               {w, h} = template.size
               -- Account for rotation in size:
               if orient == SourceMap.ORIENT_TURN_90 or orient == SourceMap.ORIENT_TURN_180
                   w, h = h, w
               {x1, y1, x2, y2} = r\bbox()
               -- Expand just outside the bounds of the region:
               x1, y1, x2, y2 = (x1 - w), (y1 - h), (x2 + w), (y2 + h)
               -- Ensure we are within the bounds of the world map:
               x1, y1, x2, y2 = math.max(x1, 0), math.max(y1, 0), math.min(x2, mw - w), math.min(y2, mh - h)
               top_left_xy = MapUtils.random_square(map, {x1, y1, x2, y2})
               apply_args = {:map, :top_left_xy, orientation: orient }
               if template\matches(apply_args)
                   template\apply(apply_args)
                   return true
           return false
       -- Function to try placement n times, returns success:
       attempt_placement_n_times = (template, n) ->
           for i=1,n
               if attempt_placement(template)
                   return true
           return false
       -- Try to create the template object using our placement routines:
       if attempt_placement_n_times(template, 100)
           -- Exit, as we have handled the first overworld component
           return true
       return false
    OldMapSeq1 = MapSequence.create {preallocate: 1}
    OldMapSeq2 = MapSequence.create {preallocate: 1}

    ------------------------- 
    -- Place easy dungeon: --
    place_dungeon = Region1.old_dungeon_placement_function(OldMapSeq1, TileSets.pebble, {1,5})
    vault = SourceMap.area_template_create(Vaults.pebble_ridge_dungeon {dungeon_placer: place_dungeon})
    if not place_feature(vault, (r) -> r.conf.is_overworld)
        return nil
    ------------------------- 

    ------------------------- 
    -- Place hard dungeon: --
    enemy_placer = (map, xy) ->
        enemy = OldMaps.enemy_generate(OldMaps.medium_animals)
        MapUtils.spawn_enemy(map, enemy, xy)
    place_dungeon = Region1.old_dungeon_placement_function(OldMapSeq2, TileSets.snake, {6,10})
    vault = SourceMap.area_template_create(Vaults.skull_surrounded_dungeon {dungeon_placer: place_dungeon, :enemy_placer})
    if not place_feature(vault, (r) -> not r.conf.is_overworld)
        return nil
    ------------------------- 

    ------------------------- 
    -- Place shop_vaults:  --
    for i=1,2
        enemy_placer = (map, xy) ->
            enemy = OldMaps.enemy_generate(OldMaps.medium_animals)
            MapUtils.spawn_enemy(map, enemy, xy)
        store_placer = (map, xy) ->
            Region1.generate_store(map, xy)
        gold_placer = (map, xy) ->
            MapUtils.spawn_item(map, "Gold", random(2,10), xy)
        vault = SourceMap.area_template_create(Vaults.shop_vault {:enemy_placer, :gold_placer, :store_placer})
        if not place_feature(vault, (r) -> true)
            return nil
    ------------------------- 


    -------------------------------
    -- Place centaur challenge   --
    enemy_placer = (map, xy) ->
        enemy = OldMaps.enemy_generate(OldMaps.harder_enemies)
        MapUtils.spawn_enemy(map, enemy, xy)
    boss_placer = (map, xy) ->
        MapUtils.spawn_enemy(map, "Dark Centaur", xy)
    item_placer = (map, xy) ->
        item = ItemUtils.item_generate(ItemGroups.enchanted_items)
        MapUtils.spawn_item(map, item.type, item.amount, xy)
    gold_placer = (map, xy) ->
        if map.rng\chance(.7) 
            MapUtils.spawn_item(map, "Gold", random(2,10), xy)
    vault = SourceMap.area_template_create(Vaults.anvil_encounter {:enemy_placer, :boss_placer, :item_placer, :gold_placer})
    if not place_feature(vault, (r) -> true)
        return nil
    -------------------------------

    -- Return the post-creation callback:
    return (game_map) ->
        OldMapSeq1\slot_resolve(1, game_map)
        OldMapSeq2\slot_resolve(1, game_map)

overworld_try_create = (rng) ->
    rng = rng or require("mtwist").create(random(0, 2 ^ 31))
    conf = OVERWORLD_CONF(rng)
    {PW,PH} = LEVEL_PADDING
    mw,mh = nil,nil
    if rng\random(0,2) == 1
        mw, mh = OVERWORLD_DIM_LESS, OVERWORLD_DIM_MORE
    else 
        mw, mh = OVERWORLD_DIM_MORE, OVERWORLD_DIM_LESS
    outer = Region.create(1+PW,1+PH,mw-PW,mh-PH)
    -- Generate regions in a large area, crop them later
    rect = {{1+PW, 1+PH}, {mw-PW, 1+PH}, {mw-PW, mh-PH}, {1+PW, mh-PH}}
    rect2 = {{1+PW, mh-PH}, {mw-PW, mh-PH}, {mw-PW, 1+PH}, {1+PW, 1+PH}}
    major_regions = RVORegionPlacer.create {rect2}
    map = SourceMap.map_create { 
        rng: rng
        size: {mw, mh}
        content: conf.wall1.id
        flags: {SourceMap.FLAG_SOLID, SourceMap.FLAG_SEETHROUGH}
        map_label: conf.map_label,
        instances: {}
        door_locations: {}
        rectangle_rooms: {}
        wandering_enabled: false
        -- For the overworld, created by dungeon features we add later:
        player_candidate_squares: {}
    }

    for subconf in *{DUNGEON_CONF(rng), OVERWORLD_CONF(rng)}
        {w,h} = subconf.size
        -- Takes region parameters, region placer, and region outer ellipse bounds:
        r = random_region_add rng, w, h, 20, spread_region_delta_func(map, rng, outer), 0,
            major_regions, outer\bbox()
        if r == nil
            return nil
        r.max_speed = 10
        r.conf = subconf
 
    -- No rvo for now

    -- Apply the regions:
    for r in *major_regions.regions
        r._points = false
        r\apply {:map, operator: (tile_operator r.conf.wall1)}

    generate_subareas(map, rng, major_regions.regions)
    map.regions = major_regions.regions

    -- Reject levels that are not fully connected:
    if not SourceMap.area_fully_connected {
        :map, 
        unfilled_selector: {matches_none: {SourceMap.FLAG_SOLID}}
        mark_operator: {add: {SourceMap.FLAG_RESERVED2}}
        marked_selector: {matches_all: {SourceMap.FLAG_RESERVED2}}
    }
        return nil

    post_creation_callback = overworld_features(map)
    if not post_creation_callback
        return nil
    overworld_spawns(map)

    game_map = generate_game_map(map)
    post_creation_callback(game_map)
    player_spawn_points = MapUtils.pick_player_squares(map, map.player_candidate_squares)
    World.players_spawn(game_map, player_spawn_points)
    return game_map

overworld_create = () ->
    for i=1,1000
        map = overworld_try_create()
        if map
            return map
        print "** MAP GENERATION ATTEMPT " .. i .. " FAILED, RETRYING **"
    error("Could not generate a viable overworld in 1000 tries!")

return {
    :overworld_create, :generate_game_map
}
