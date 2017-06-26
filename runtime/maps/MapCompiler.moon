-- Box2D generation utilities
B2GenerateUtils = require "maps.B2GenerateUtils"
GenerateUtils = require "maps.GenerateUtils"
GeometryUtils = require "maps.GeometryUtils"
DebugUtils = require "maps.DebugUtils"
SourceMap = require "core.SourceMap"
MapRegionShapes = require("maps.MapRegionShapes")
Map = require "core.Map"
MapUtils = require "maps.MapUtils"
World = require "core.World"
Vaults = require "maps.Vaults"

import MapRegion, combine_map_regions, map_regions_bbox
    from require("maps.MapRegion")

import ConnectedRegions, FilledRegion
    from require "maps.MapElements"

make_polygon_points = (rng, w, h, n_points) ->
    return GenerateUtils.skewed_ellipse_points(rng, {0,0}, {30, 30}, rng\random(4,12))

default_fill = () =>
    -- Fill the room:
    i = 1
    @for_all_nodes (node) =>
        @apply node, {
            operator: {
                remove: SourceMap.FLAG_SOLID
                add: SourceMap.FLAG_SEETHROUGH
                content: if i%2 == 1 then @tileset.floor else @tileset.floor_alt
                group: i
            }
        }
        SourceMap.perimeter_apply {map: @map,
            candidate_selector: {matches_all: SourceMap.FLAG_SOLID}, inner_selector: {matches_none: SourceMap.FLAG_SOLID, matches_group: i}
            operator: {
                content: if i%2 == 1 then @tileset.wall else @tileset.wall_alt
            }
        }
        i += 1
    SourceMap.erode_diagonal_pairs {map: @map, rng: @rng, selector: {matches_all: SourceMap.FLAG_SOLID}}
    if not SourceMap.area_fully_connected {
        map: @map, 
        unfilled_selector: {matches_none: {SourceMap.FLAG_SOLID}}
        mark_operator: {add: {SourceMap.FLAG_RESERVED2}}
        marked_selector: {matches_all: {SourceMap.FLAG_RESERVED2}}
    }
        log_verbose("ABORT: connection check failed")
        return false
    return true
MapCompiler = newtype {
    init: (args) =>
        -- Result memoizing table 
        @result = {}
        @operators = {}
        @label = assert (args.label or @label)
        @root_node = assert (args.root or @root_node) -- Take from base class
        @rng = assert args.rng
        -- Maps from node -> data
        @_children = {}
        @_regions = {}
        @_combined_region = {}
        @_next_group_id = 1
        @fill_function = args.fill_function or rawget(@, "fill_function") or default_fill
        @prepare(args)
    add: (selector, operator) =>
        append @operators, {:selector, :operator}

    -- Override to handle explicitly set spawn points
    get_player_spawn_points: () =>
        log_verbose "get_player_spawn_points #{@label} #{#World.players}"
        return for player in *World.players
            log_verbose "get_player_spawn_points #{player.name}"
            {x, y} = MapUtils.random_square(@map, nil)
            {x*32+16,y*32+16}

    get_map_region: (node) =>
        assert @_regions[node], "Must run compile_map_topology on node first!"
        regions = for child in *@_children[node]
            region = @get_map_region(child)
        return combine_map_regions(regions)

    _recalculate_perimeter: () =>
        -- Detect the perimeter, important for the winding-tunnel algorithm.
        SourceMap.perimeter_apply {map: @map,
            candidate_selector: {matches_all: SourceMap.FLAG_SOLID}, inner_selector: {matches_none: SourceMap.FLAG_SOLID}
            operator: {add: SourceMap.FLAG_PERIMETER}
        }
        SourceMap.perimeter_apply {map: @map,
            candidate_selector: {matches_none: {SourceMap.FLAG_SOLID}}, 
            inner_selector: {matches_all: {SourceMap.FLAG_PERIMETER, SourceMap.FLAG_SOLID}}
            operator: {add: Vaults.FLAG_INNER_PERIMETER}
        }
    _generate_shape: (scheme, x, y, w, h) =>
        switch scheme
            when 'deformed_ellipse'
                points = @rng\random(5, 15)
                polygon = GenerateUtils.skewed_ellipse_points(@rng, {x,y}, {w, h}, points)
                return {polygon}
            else
                parts = scheme\split(':')
                name = parts[1]
                if parts[2] == 'convex'
                    polygon = MapRegionShapes.get_shape_convex_hull(name, x, y, w, h)
                    return {polygon}
                else
                    polygons = MapRegionShapes.get_shape_polygons(name, x, y, w, h)
                    return polygons
    _spread_regions: (scheme, regions) =>
        timer = timer_create()
        switch scheme
            when 'box2d'
                B2GenerateUtils.spread_map_regions {
                    rng: @rng
                    :regions 
                    n_iterations: 100
                    mode: 'towards_center'
                    clump_once_near: true
                }
            when 'box2d_solid_center'
                B2GenerateUtils.spread_map_regions {
                    rng: @rng
                    :regions 
                    fixed_polygons: @_generate_shape('deformed_ellipse', 0,0,4,4)
                    n_iterations: 100
                    mode: 'towards_fixed_shapes'
                    clump_once_near: true
                }
            else
                error("Unexpected")
        log_verbose "Spread regions time: #{timer\get_milliseconds()}ms"
    _connect_regions: (scheme, regions) =>
        switch scheme
            when 'direct_light'
                timer = timer_create()
                B2GenerateUtils.connect_map_regions {
                    rng: @rng
                    :regions 
                    n_connections: 2
                }
                log_verbose "Connect regions time: #{timer\get_milliseconds()}ms"
            when 'direct'
                timer = timer_create()
                B2GenerateUtils.connect_map_regions {
                    rng: @rng
                    :regions 
                    n_connections: #regions * 2
                }
                log_verbose "Connect regions time: #{timer\get_milliseconds()}ms"
            when 'none'
                nil
            else
                error("Unexpected")

    _flatten: (l, accum={}) =>
        if getmetatable(l)
            append accum, l
        else
            for elem in *l
                @_flatten(elem, accum)
        return accum
    -- Sets node_children and node_regions
    _prepare_map_topology: (node) =>
        map_regions = {}
        children = {}
        combined_region = nil
        switch getmetatable(node)
            when ConnectedRegions
                {:name, :regions, :connection_scheme, :spread_scheme} = node
                child_regions = for region_node in *@_flatten(regions)
                    append children, region_node
                    @_prepare_map_topology(region_node)
                    -- TODO smarter spreading
                    x, y = @rng\random(-20, 20), @rng\random(-20,20)
                    -- Can translate entire combined region; translates all subregions as well:
                    combined_region = @_combined_region[region_node]
                    combined_region\translate(x, y)
                    combined_region
                @_spread_regions(spread_scheme, child_regions)
                @_connect_regions(connection_scheme, child_regions)
                combined_region = combine_map_regions(child_regions)
                DebugUtils.visualize_map_regions {regions: child_regions, title: "After connection / spread"}

            when FilledRegion
                {:name, :shape, :size} = node
                {w, h} = size
                combined_region = MapRegion.create @_generate_shape(shape,0,0,w,h)
                append map_regions, combined_region
                DebugUtils.visualize_map_regions {regions: map_regions, title: "FilledRegion"}
            else
                error("Unknown node")
        -- Regions 'owned' by this node
        @_regions[node] = map_regions
        @_children[node] = children
        @_combined_region[node] = assert combined_region, "Need combined_region ~= nil!"
    get_node_owned_regions: (node) => @_regions[node]
    get_node_children: (node) => @_children[node]
    get_node_total_region: (node) => @_combined_region[node]

    fill_unconnected: () =>
        {w,h} = @map.size
        SourceMap.area_fill_unconnected {
            map: @map 
            seed: {w/2, h/2}
            unfilled_selector: {matches_none: {SourceMap.FLAG_SOLID}}
            mark_operator: {add: {SourceMap.FLAG_RESERVED2}}
            marked_selector: {matches_all: {SourceMap.FLAG_RESERVED2}}
            fill_operator: {content: @tileset.wall, add: SourceMap.FLAG_SOLID, remove: SourceMap.FLAG_SEETHROUGH}
        }
    apply: (node, args) =>
        assert not args.map, "Passing map here redundant!"
        args.map = @map
        for region in *@_regions[node]
            region\apply(args)

    for_all_nodes: (func) =>
        recurse = (node) ->
            func(@, node)
            for child in * @_children[node]
                recurse(child)
        recurse(@root_node)

    _prepare_source_map: (label, padding, content) => 
        -- Correct map topology:
        all_regions = {}
        @for_all_nodes (node) =>
            for region in *@_regions[node]
                append all_regions, region
        --bbox = map_regions_bbox({@_combined_region[@root_node]})
        bbox = map_regions_bbox(all_regions)
        --total_region = @get_node_total_region(@root_node)
        --total_region\translate(-bbox[1] + padding, -bbox[2] + padding)
        -- TODO remove this when confident about the code
        -- Assert that our polygons fit within our created source map bounds:
        w, h = bbox[3] - bbox[1], bbox[4] - bbox[2] 
        -- TODO DEBUG-ONLY CHECK Really make sure we are drawing in correct bounds
        @for_all_nodes (node) =>
            for region in *@_regions[node]
                region\translate(-bbox[1] + padding, -bbox[2] + padding)
                for polygon in *region.polygons
                    for {x, y} in *polygon
                        assert x >= padding - 0.1 and x <= w+padding +0.1, "pad=#{padding}, #{x}, #{w}"
                        assert y >= padding - 0.1 and y <= h+padding +0.1 , "pad=#{padding}, #{y}, #{h}"
        print("WIDTH", w, "HEIGHT", h)
        @map = SourceMap.map_create {
            rng: @rng
            :label
            size: {w + padding*2, h + padding*2}
            flags: SourceMap.FLAG_SOLID
            instances: {}
            :content 
        }
        if not @fill_function()
            return false
        @_recalculate_perimeter()
        return true

    random_square: (area = nil) => 
        return MapUtils.random_square(@map, area, {matches_none: {SourceMap.FLAG_HAS_OBJECT, Vaults.FLAG_HAS_VAULT, SourceMap.FLAG_SOLID}})
    random_square_not_near_wall: (area = nil) =>
        return MapUtils.random_square(@map, area, {matches_none: {Vaults.FLAG_INNER_PERIMETER, SourceMap.FLAG_HAS_OBJECT, Vaults.FLAG_HAS_VAULT, SourceMap.FLAG_SOLID}})

    -- Creates @map, ready to be filled
    prepare: (args) => 
        success = false
        while not success
            table.clear @_regions
            table.clear @_children
            table.clear @_combined_region
            label = assert (args.label or @label), "Should have map 'label'"
            padding = args.padding or 10
            content = args.content or (if rawget(self, 'tileset') then @tileset.wall else 0)
            @_prepare_map_topology(@root_node)
            success = @_prepare_source_map(label, padding, content)
    -- Creates a game map
    compile: (C) =>
        return Map.create {
            map: @map
            label: @map.label
            instances: @map.instances
            wandering_enabled: true
        }
}

main = (raw_args) ->
    -- AreaTemplate -> MapAreaSet -> Map
    area = ConnectedRegions {
        regions: for i=1,3
            FilledRegion {
                shape: 'deformed_ellipse'
                size: {(i+5) * 10, (i+5) * 10}
            }
        connection_scheme: 'direct'
        spread_scheme: 'box2d'
    }
    rng = require('mtwist').create(os.time())
    compiler = MapCompiler.create {
        label: "Demo"
        :rng,
        root: area
    }
    
    i = 1
    compiler\for_all_nodes (node) =>
        @apply node, {
            operator: {
                remove: SourceMap.FLAG_SOLID
                add: SourceMap.FLAG_SEETHROUGH
                content: i
            }
        }
        i += 1
    DebugUtils.enable_visualization(800, 600)
    DebugUtils.debug_show_source_map(compiler.map)

return {:MapCompiler, :main}
