function TestCases.SerializeBuffer_bindings()
    local SB = import "core.io.SerializeBuffer"
    lunit.assert(SB)
    lunit.assert(SB.create)
    local sb = SB.create()
    lunit.assert(sb)

    sb:write_raw('Hello World!')
    lunit.assert_equal('Hello World!', sb.raw_buffer)
    lunit.assert_equal('Hello World!', sb:read_raw(#'Hello World!'))
end
