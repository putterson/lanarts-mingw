/*
 * lua_image_tests.cpp:
 *  Test ldraw::image bindings in lua
 */

#include <luawrap/testutils.h>
#include <luawrap/LuaValue.h>
#include <luawrap/calls.h>
#include <luawrap/functions.h>

#include <common/unittest.h>

#include "../lua/lua_ldraw.h"

#include "../Image.h"
#include "../lua/lua_image.h"
#include "../lua/lua_drawable.h"

static ldraw::Image img_identity_function(const ldraw::Image& img) {
	return img;
}

static void SLB_sanity_test() {
	using namespace ldraw;

	TestLuaState L;
	LuaValue globals = LuaValue::globals(L);
	ldraw::lua_register_image(L, globals);

	const char* code = "function checksanity()\n"
			"end\n";

	lua_assert_valid_dostring(L, code);
	{
		globals.get(L, "checksanity").push();
		luawrap::call<void>(L);
	}
	{
		globals.get(L, "checksanity").push();
		luawrap::call<void>(L, (int) 1); // Smoke test
	}
	{
		Image image;
		globals.get(L, "checksanity").push();
		luawrap::call<void>(L, image); // Smoke test
	}
	{
		using namespace ldraw;
		Image image;
		image.draw_region() = BBoxF(0, 0, 1, 1);

		luawrap::push_function(L, img_identity_function);
		BBoxF returned = luawrap::call<Image>(L, image).draw_region();
		UNIT_TEST_ASSERT(image.draw_region() == returned);
	}

	L.finish_check();
}

static void lua_image_bind_test() {
//	using namespace ldraw;
//
//	lua_State* L = lua_open();
//	{
//		Image image;
//		image.draw_region() = BBoxF(0, 0, 10, 10);
//		Drawable drawable(new Image(image));
//
//		m.set("assert", FuncCall::create(unit_test_assert));
//
//		{
//			const char* code2 =
//					"function testImgProperties(img)\n"
//							"assert('width does not exist', img.width ~= nil)\n"
//							"assert('height does not exist', img.height ~= nil)\n"
//							"assert('draw does not exist', img.draw ~= nil)\n"
//							"assert('size does not exist', img.size ~= nil)\n"
//							"assert('duration is not nil', img.duration == nil)\n"
//							"assert('animated is not false', img.animated == false)\n"
//							"end\n";
//
//			lua_assert_valid_dostring(L, code2);
//			{
//				LuaCall<void(const Image&)> call(L, "testImgProperties");
//				call(image);
//			}
//			//Drawable should be equivalent
//			{
//				LuaCall<void(const Drawable&)> call(L, "testImgProperties");
//				call(drawable);
//			}
//		}
//		{
//			const char* code =
//					"function testImgWidth(img)\n"
//							"assert('widths do not match', img.width == 10)\n"
//							"assert('heights do not match', img.height == 10)\n"
//							"assert('duration is not nil', img.duration == nil)\n"
//							"assert('animated is not false', img.animated == false)\n"
//							"end\n";
//
//			lua_assert_valid_dostring(L, code);
//			{
//				LuaCall<void(const Image&)> call(L, "testImgWidth");
//				call(image);
//			}
//			//Drawable should be equivalent
//			{
//				LuaCall<void(const Drawable&)> call(L, "testImgWidth");
//				call(drawable);
//			}
//		}
//
//	}
//
//	UNIT_TEST_ASSERT(lua_gettop(L) == 0);
//	lua_close(L);
}

void lua_image_tests() {
	UNIT_TEST(SLB_sanity_test);
	UNIT_TEST(lua_image_bind_test);

}

