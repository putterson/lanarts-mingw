local Tmpl = import "@codegen.Templates"
local Types = import "@codegen.FieldTypes"

local function make_slice(data) return {data, 0} end

function TestCases.type_type_builder()
    local BTypes = Types.builtin_types
    local T = Tmpl.type_parse [[
        a, b : int
        c, d : float
    ]]
    local expected = {a=BTypes.int,b=BTypes.int,c=BTypes.float,d=BTypes.float}
    for field in values(T.fields) do
        assert(field.name and field.type and expected[field.name] == field.type)
    end
    -- Test __index
    local index_func = Tmpl.callstring('return ' .. Tmpl.metamethods.__index(T))
    for i, k in ipairs {"a", "b", "c", "d"} do
        local data = {1, 2, 3, 4}
        assert(index_func(make_slice(data), k) == data[i])
    end
    -- Test __newindex
    local newindex_func = Tmpl.callstring('return ' .. Tmpl.metamethods.__newindex(T))
    for i, k in ipairs {"a", "b", "c", "d"} do
        local data = {}
        newindex_func(make_slice(data), k, i)
        assert(data[i] == i)
    end
    -- Test create
    local meta = Tmpl.compile_type(T)
    local val = meta.create(1,2,3,4)
    for i, k in ipairs {"a", "b", "c", "d"} do
        assert(val[k] == val[1][i])
    end
    local val = meta.create(0,0,0,0)
    for i, k in ipairs {"a", "b", "c", "d"} do
        assert(val[k] == 0)
        val[k] = i
        assert(val[k] == i)
    end
end

function TestCases.test_typedef()
    local t = typedef [[
        x, y : int
    ]]
    local p = t.create(1,2)
    print(p)
    assert(p.x == 1 and p.y == 2)
    assert(not pcall(function()
        p.x = 1.1 -- Should error!
    end))
    assert(not pcall(function()
        p.y = 1.1 -- Should error!
    end))
    p.x, p.y = 2, 1
    assert(p.x == 2 and p.y == 1)
end

function TestCases.test_can_define_methods()
    local t = typedef [[
        dummy : int
    ]]
    local calledMethod = false
    local val = t.create(1)
    function t:some_method()
        calledMethod = true
    end
    t:some_method()
    assert(calledMethod)
end

function TestCases.test_embedded_type()
    local N = {}
    typedef(N) "Foo" [[
        foo : int
        bar : string
    ]]
    typedef(N) "Bar" [[
        Foo
    ]]
    local b = N.Bar.create(N.Foo.create(4, 'test'))
    assert(b.Foo.foo == 4)
    assert(b.Foo.bar == 'test')
    assert(b.foo == 4)
    assert(b.bar == 'test')
    print(b)
    print(b.Foo)
end