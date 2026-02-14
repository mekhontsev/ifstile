#include "js_engine.h"

#include "../quickjs/quickjs.h"

#include "visible_blocks.h"
#include "gui.h"
#include "oper_block.h"
#include "ifs_list.h"

static JSValue js_ifs_id(
	JSContext* ctx,
	JSValueConst,
	int argc,
	JSValueConst* argv)
{
	const oper_block* b = nullptr;

	if (argc == 0) {
		b = get_vb().get_cur_block();
	} else {

		if (!JS_IsString(argv[0])) {
			return JS_ThrowTypeError(ctx, "Argument must be a string");
		}

		std::string str;
		let* s = JS_ToCString(ctx, argv[0]);
		str = s;
		JS_FreeCString(ctx, s);

		b = ifs_list_get().find_block2(str, str);
	}

	return JS_NewInt32(ctx, b ? (int32_t)b->m_block_id : 0);

}

void js_reg_ifs(JSContext* ctx, JSValue& global_obj)
{
	auto ifs_obj = JS_GetPropertyStr(ctx, global_obj, "ifs");
	IMS_SCOPE([&] {JS_FreeValue(ctx, ifs_obj); });

	JS_SetPropertyStr(ctx, ifs_obj, "id",
		JS_NewCFunction(ctx, js_ifs_id, nullptr, 1));
};