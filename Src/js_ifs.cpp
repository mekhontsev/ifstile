#include "js_engine.h"

#include "../quickjs/quickjs.h"

#include "visible_blocks.h"
#include "gui.h"
#include "oper_block.h"
#include "ifs_list.h"
#include "columns.h"
#include "data_column.h"

using reg_function = void (*)(JSContext*, JSValue&);
void js_add_reg_func(reg_function f);
void js_set_arr_size(JSContext* ctx, JSValue& arr, size_t size);
int64_t js_get_arr_size(JSContext* ctx, const JSValue& v);
bool js_get_int64(JSContext* ctx, int64_t* pres, const JSValue& val);

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
			return JS_ThrowTypeError(ctx, "Argument 1 must be a string");
		}

		std::string str;
		let* s = JS_ToCString(ctx, argv[0]);
		str = s;
		JS_FreeCString(ctx, s);

		b = ifs_list_get().find_block2(str, str);
	}

	return JS_NewInt32(ctx, b ? (int32_t)b->m_block_id : 0);

}

static JSValue js_ifs_blocks(
	JSContext* ctx,
	JSValueConst,
	int,
	JSValueConst*)
{
	let& b = ifs_list_get().m_blocks;
	let sz = b.size();
	JSValue arr = JS_NewArray(ctx);
	js_set_arr_size(ctx, arr,sz);
	for (size_t i = 0; i < sz; ++i) {
		let vi = JS_NewInt64(ctx, (int64_t)b[i]);
		JS_SetPropertyUint32(ctx, arr, (uint32_t)i, vi);
	}
	return arr;
}

static oper_block* get_block(JSContext* ctx, JSValue v)
{
	let& lst = ifs_list_get();
	int64_t i64;
	if (!js_get_int64(ctx, &i64, v) ||
		i64 < 0 || (size_t)i64 >= lst.m_id2data.size())
	{
		return nullptr;
	}
	return ifs_list_get().get_block((block_id_t)i64);
}

static column_id::ECID get_field(JSContext* ctx, JSValue v, std::string& err_field)
{
	let s2id = columns::get().m_str2id;
	let* s = JS_ToCString(ctx, v);
	IMS_SCOPE([&] {JS_FreeCString(ctx, s); });
	let it = s2id.find(s);
	if (it != s2id.end()) {
		return it->second;
	}
	err_field = s;
	return column_id::NUM_COLS;
}

static bool get_string(JSContext* ctx, JSValue v, std::string& dst)
{
	if (!JS_IsString(v)) {
		return false;
	}
	let* s = JS_ToCString(ctx, v);
	IMS_SCOPE([&] {JS_FreeCString(ctx, s); });
	dst = s;
	return true;
}


static JSValue js_ifs_get(
	JSContext* ctx,
	JSValueConst,
	int argc,
	JSValueConst* argv)
{
	if (argc < 2) {
		return JS_UNDEFINED;
	}

	let* b = get_block(ctx, argv[0]);
	if (!b) {
		return JS_ThrowTypeError(ctx, "Invalid argument 1");
	}

	if (!JS_IsArray(argv[1])) {
		return JS_ThrowTypeError(ctx, "Invalid argument 2");
	}

	let sz = js_get_arr_size(ctx, argv[1]);

	JSValue arr = JS_NewArray(ctx);
	js_set_arr_size(ctx, arr, sz);
	std::string str;
	for (uint32_t i = 0; i < sz; ++i) {
		auto vi = JS_GetPropertyUint32(ctx, argv[1], i);
		IMS_SCOPE([&] {JS_FreeValue(ctx, vi); });

		if (!JS_IsString(vi)) {
			return JS_ThrowTypeError(ctx, "Array entries must be strings");
		}

		let cid = get_field(ctx, vi, str);
		if (cid == column_id::NUM_COLS) {
			return JS_ThrowTypeError(ctx, "Column '%s' is not defined", str.c_str());
		}

		let& c = data_column::g_cols[cid];
		let* u = b->m_calc_data.get();

		JSValue di = JS_UNDEFINED;
		IMS_SCOPE([&] {
			JS_SetPropertyUint32(ctx, arr, (uint32_t)i, di);
		});

		if (c.is_need_search() && !u) {
			continue;//undefined
		};

		if (c.get_int) {
			di = JS_NewInt64(ctx, c.get_int(*b, *u));
			continue;
		};

		if (c.get_float) {
			di = JS_NewNumber(ctx, c.get_float(*b, *u));
			continue;
		};

		assert(c.to_string);
		str.clear();
		c.to_string(*b, *u, std::back_inserter(str));
		di = JS_NewString(ctx, str.c_str());
	}
	return arr;
}

static JSValue js_ifs_set(
	JSContext* ctx,
	JSValueConst,
	int argc,
	JSValueConst* argv)
{
	if (argc < 3) {
		return JS_ThrowTypeError(ctx, "Invalid number of arguments");
	}

	auto* b = get_block(ctx, argv[0]);
	if (!b) {
		return JS_ThrowTypeError(ctx, "Invalid argument 1");
	}

	std::string str;
	let cid = get_field(ctx, argv[1], str);
	if (cid == column_id::NUM_COLS) {
		return JS_ThrowTypeError(ctx, "Column '%s' is not defined", str.c_str());
	}

	switch (cid)
	{
	case column_id::CH:
	case column_id::HD:
	{
		int64_t res{};
		if (0 != JS_ToInt64(ctx, &res, argv[2])) {
			return JS_ThrowTypeError(ctx, "Argument 3 must be an int");
		};

		if (cid == column_id::CH) {
			get_vb().set_checked(b, res != 0);
		} else {
			b->m_flags.hidden = res != 0;
		}
		break;
	}
	case column_id::Name:
	{
		if (!get_string(ctx, argv[2], b->m_name)) {
			return JS_ThrowTypeError(ctx, "Argument 3 must be a string");
		}
		break;
	}
	default:
		return JS_ThrowTypeError(ctx,
			"The column cannot be modified.");
	}

	return JS_UNDEFINED;
}

void js_reg_ifs_ex(JSContext* ctx, JSValue& ifs_obj)
{
	struct func_entry
	{
		const char* name;
		JSCFunction* f;
		int num_args;
	};

	const std::array<func_entry, 4> farr = {{
		{"id",		js_ifs_id,		1},
		{"blocks",	js_ifs_blocks,	0},
		{"get",		js_ifs_get,		2},
		{"set",		js_ifs_set,		3}
	}};

	for (let& q : farr) {
		JS_SetPropertyStr(ctx, ifs_obj, q.name,
			JS_NewCFunction(ctx, q.f, nullptr, q.num_args));
	}
};

void js_reg_ifs() 
{
	js_add_reg_func(js_reg_ifs_ex);
}
