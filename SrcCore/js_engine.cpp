// This file is part of IFStile project
// Copyright (C)2026 Dmitry Mekhontsev <mekhontsev@gmail.com>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "js_engine.h"

#include "../quickjs/quickjs.h"

#include "ifs_data_text.h"
#include "oper_block.h"
#include "ifs_list.h"
#include "error_helper.h"
#include "ims_keywords.h"
#include "ims_file.h"

#include "ims_val.h"
#include "eval_pool.h"
#include "pool_ptr.h"

std::vector<js_engine::reg_function> js_engine::s_js_export;


static int64_t js_get_arr_size(JSContext* ctx, JSValue v)
{
	int64_t sz = 0;
	if (JS_GetLength(ctx, v, &sz)) {
		return 0;
	}
	return sz;
};

static bool get_int64(JSContext* ctx, int64_t* pres, JSValue val)
{
	double res;
	if (0 != JS_ToFloat64(ctx, &res, val)) {
		return false;
	}

	let i = static_cast<int64_t>(res);
	if (double(i) != res) {
		return false;
	}

	*pres = i;
	return true;
}

struct js_arr_enumerator
{
	std::vector<JSValue> m_values;

	struct info
	{
		size_t start = 0;		//start of the array
		size_t length = 0;		//length of the array
		size_t num_ref = 0;		//number of references from other locations
		union {
			size_t index = ims_max; //ims_max or variable it is equal to
			ims_val* val;
		};
	};

	///////////////////////////////////////////////////////////////////////////

	bool empty() const
	{
		assert(m_values.empty() == m_map.empty());
		return m_values.empty();
	}

	info* find(const JSValue& v)
	{
		auto it = m_map.find(JS_VALUE_GET_PTR(v));
		if (it == m_map.end()) return nullptr;
		return &it->second;
	}

	const info* find(const JSValue& v) const
	{
		return const_cast<js_arr_enumerator*>(this)->find(v);
	}

	size_t set_index(size_t next_var)
	{
		for (auto& q : m_map) {
			if (q.second.num_ref <= 1 || q.second.index != ims_max) {
				continue;
			}
			q.second.index = next_var++;
		}
		return next_var;
	}

	void clear(JSContext* ctx)
	{
		m_process_queue.clear();
		m_map.clear();

		for (let& v : m_values) {
			JS_FreeValue(ctx, v);
		}
		m_values.clear();
	};

	static const ims_val* convert(JSValue vi, JSContext* ctx)
	{
		if (JS_IsString(vi)) {
			let* s = JS_ToCString(ctx, vi);
			IMS_SCOPE([&] {JS_FreeCString(ctx, s); });
			return eval_pool::ep.get_string(s);
		}

		if (JS_IsBigInt(vi)) {
			if (JS_VALUE_GET_TAG(vi) == JS_TAG_SHORT_BIG_INT) {
				return eval_pool::ep.get_scalar_int(JS_VALUE_GET_SHORT_BIG_INT(vi));
			}

			const char* s = JS_ToCString(ctx, vi);
			IMS_SCOPE([&] {JS_FreeCString(ctx, s); });

			int64_t i64;
			if (boost::conversion::try_lexical_convert(s, i64)) {
				return eval_pool::ep.get_scalar_int(i64);
			}

			auto* a = eval_pool::ep.get_scalar_big_rational();
			*a->p_b() = ims_integer_big(s);
			return a;
		}

		if (JS_IsNumber(vi)) {
			double f64;
			JS_ToFloat64(ctx, &f64, vi);
			return eval_pool::ep.get_scalar_real(f64);
		}

		//the type is not supported (bool, object, function, etc...)
		return nullptr;
	}

	const ims_val* load_arr_tree(JSValue vx, JSContext* ctx)
	{
		assert(empty());

		IMS_SCOPE([&] {clear(ctx); });

		process_arrays(vx, ctx);

		for (auto& q : m_map) {
			q.second.val = eval_pool::ep.get_vector(q.second.length);
		}

		IMS_SCOPE([&] {
			for (auto& q : m_map) {
				eval_pool::ep.release(q.second.val);
			}
		});

		for (auto& q : m_map) {
			let& m = q.second;
			auto** arr = m.val->p_v();
			for (size_t i = 0; i < m.length; ++i) {
				let& vi = m_values[m.start + i];
				auto* mi = find(vi);
				if (mi) {//another array
					arr[i] = mi->val;
					mi->val->add_ref();
				} else {
					arr[i] = convert(vi, ctx);
				}
			}
		}

		auto* rm = find(vx);
		if (!rm)return nullptr;

		auto* ret = rm->val;
		ret->add_ref();
		return ret;
	}

	//recursively add arrays referenced by the operator
	//so that we can then create our own operators for them
	void process_arrays(JSValue vx, JSContext* ctx)
	{
		assert(m_process_queue.empty());
		IMS_SCOPE([&] {m_process_queue.clear(); });

		m_process_queue.emplace_back(vx);

		while (!m_process_queue.empty()) {

			auto v = m_process_queue.back();
			m_process_queue.pop_back();

			if (!JS_IsArray(v)) {
				continue;
			}

			auto res = m_map.emplace(JS_VALUE_GET_PTR(v), info());
			auto& m = res.first->second;
			++m.num_ref;


			if (!res.second) {
				continue;//already visited
			}
			m.index = ims_max;
			m.length = js_get_arr_size(ctx, v);
			m.start = m_values.size();
			m_values.resize(m_values.size() + m.length, JS_UNDEFINED);

			for (uint32_t i = 0; i < m.length; ++i) {
				let vi = JS_GetPropertyUint32(ctx, v, i);
				m_values[m.start + i] = vi;
				m_process_queue.push_back(vi);
			}
		}
	}

private:

	std::vector<JSValue> m_process_queue;

	//there are only arrays here
	ankerl::unordered_dense::map<void*, info> m_map;
};

////////////////////////////////////////////////////////////////////////////
//information when processing the current block
struct js_aifs_block
{
	js_aifs_block(JSContext* ctx) : m_ctx{ ctx } {};
	~js_aifs_block() { clear9(); };

	JSContext* m_ctx = nullptr;

	//module's export
	JSValue m_export = JS_UNDEFINED;
	JSValue m_constructor = JS_UNDEFINED;

	js_arr_enumerator m_enumerator;

	JSValue get_exports_entry(const char* v) 
	{
		return JS_IsObject(m_export) ?
			JS_GetPropertyStr(m_ctx, m_export, v) :
			JS_UNDEFINED;
	}

	bool create_blocks(
		JSValue aifs,
		read_state& rs,
		ifs_list& lst)
	{
		assert(m_blocks2.empty());
		IMS_SCOPE([&] {m_blocks2.clear(); });

		if (JS_IsUndefined(aifs)) {
			return true;//JS doesn't export blocks - that's fine
		}

		if (!JS_IsArray(aifs)) {
			if (!add_block3(aifs, rs, lst)) {
				return false;
			}
			return true;
		}

		let num_blocks = js_get_arr_size(m_ctx, aifs);
		for (uint32_t i = 0; i < num_blocks; ++i) {

			auto v = JS_GetPropertyUint32(m_ctx, aifs, i);
			IMS_SCOPE([&] {JS_FreeValue(m_ctx, v); });

			if (!add_block3(v, rs, lst)) {
				return false;
			}
		}
		return true;
	};

	bool init_constructor(pool_ptr& data)
	{
		assert(!data);

		auto c = get_exports_entry("$constructor");
		IMS_SCOPE([&] {JS_FreeValue(m_ctx, c); });
		if (JS_IsUndefined(c)) {
			return true;//it's ok
		}

		if (!JS_IsArray(c) || js_get_arr_size(m_ctx, c) != 2) {
			ims_error("$constructor must be a 2 elements array");
			return false;
		}

		assert(JS_IsUndefined(m_constructor));
		m_constructor = JS_GetPropertyUint32(m_ctx, c, 0);
		if (!JS_IsFunction(m_ctx, m_constructor)) {
			JS_FreeValue(m_ctx, m_constructor);	m_constructor = JS_UNDEFINED;
			ims_error("$constructor[0] must be a function");
			return false;
		}

		auto v1 = JS_GetPropertyUint32(m_ctx, c, 1);
		IMS_SCOPE([&] {JS_FreeValue(m_ctx, v1); });

		data.reset(m_enumerator.load_arr_tree(v1, m_ctx));
		if (!data ||
			!data->is(ims_val_b::ETP::vector, ims_val_b::EST::other))
		{
			ims_error("$constructor[1] must be an array of controls");
			return false;
		}

		return true;
	}

	struct init_func_info
	{
		JSValue v;
		size_t unk_id;//string identifier
	};

	//references to all $init functions for blocks
	//duplicates are allowed
	//stored as long as the file exists
	std::vector<init_func_info> m_init_funcs;


	//all blocks that have already been added (search for duplicates)
	UNAMESPACE::unordered_map<void*, oper_block*> m_blocks2;

	struct add_info
	{
		oper_block** ppb;
		JSValue v;//does not store a reference
	};

	std::vector<add_info> m_parr;//temporary

	std::vector<JSValue> m_js_vars;

	////////////////////////////////////////////////////////////////////////////

	void clear9()
	{
		assert(m_enumerator.empty());
		assert(m_js_vars.empty());
		assert(m_blocks2.empty());
		assert(m_parr.empty());

		////////////////////////////////////////////////////////////////////////
		for (auto& e : m_init_funcs) {
			JS_FreeValue(m_ctx, e.v);
		}
		m_init_funcs.clear();

		JS_FreeValue(m_ctx, m_export); m_export = JS_UNDEFINED;
		JS_FreeValue(m_ctx, m_constructor); m_constructor = JS_UNDEFINED;
	}

	bool get_rational64(const js_arr_enumerator::info& m, int64_t* n = nullptr) const
	{
		if (m.length != 3)return false;

		auto jlast = m_enumerator.m_values[m.start + 2];

		if (!JS_IsString(jlast))return false;
		{
			let* s = JS_ToCString(m_ctx, jlast);
			bool is_ok = s[0] == '/' && s[1] == 0;
			JS_FreeCString(m_ctx, s);
			if (!is_ok)return false;
		}

		int64_t rat[2];
		for (size_t i = 0; i < 2; ++i) {
			if (!get_int64(m_ctx, &rat[i], m_enumerator.m_values[m.start + i])) {
				return false;
			}
		}

		if (n) {
			n[0] = rat[0];
			n[1] = rat[1];
		}
		return true;
	}

	bool get_rational64(JSValue v, int64_t* n = nullptr) const
	{
		int64_t number;
		if (get_int64(m_ctx, &number, v)) {
			if (n) {
				n[0] = number;
				n[1] = 1;
			}
			return true;
		}

		let* it = m_enumerator.find(v);
		if (!it) {
			return false;
		}
		return get_rational64(*it, n);
	}

	bool add_block3(
		JSValue block_def,
		read_state& rs,
		ifs_list& lst);

	bool parse_var(
		size_t idx_var,
		JSValue jv,
		size_t x,
		oper_block& b,
		read_state& rs,
		ims_identifiers& unk);


	bool create_vars(
		oper_block& b,
		JSValue block_def,
		read_state& rs,
		ifs_list& lst);

};



static bool is_int32(int64_t v)
{
	using N32 = std::numeric_limits<uint32_t>;
	return	v >= N32::min() && v <= N32::max();
}


static JSValue js_new_i64_value(JSContext* ctx, int64_t v)
{
	//TODO: check bigint
	return JS_NewInt64(ctx, v);
}

static void js_set_arr_size(JSContext* ctx, JSValue arr, size_t size)
{
	JS_SetLength(ctx, arr, (int64_t)size);
}


//print error
static void js_get_error(JSValue e, JSContext* ctx, std::string& err_string)
{
	if (!JS_IsNull(e)){
		let* err = JS_ToCString(ctx, e);
		IMS_SCOPE([&] {JS_FreeCString(ctx, err); });
		if (err) {
			err_string += err;
			err_string += "\n";
		}
	}

	JSValue stack = JS_GetPropertyStr(ctx, e, "stack");
	IMS_SCOPE([&] {JS_FreeValue(ctx, stack); });

	if (!JS_IsUndefined(stack)) {
		let* err = JS_ToCString(ctx, stack);
		IMS_SCOPE([&] {JS_FreeCString(ctx, err); });
		if (err)err_string += err;
	}

	JS_ResetUncatchableError(ctx);
};

static JSValue create_js_value(
	JSContext* ctx,
	const operator_ptr& p,
	bool create_vectors)
{
	bool is_rational = false;
	double fv;
	int64_t iv;
	int64_t ivd;

	let& b = *p.a;
	let& h = p.h;


	bool res = b.get_val(h, is_rational, fv, iv, ivd);

	JSValue val = JS_UNDEFINED;

	if (res) {

		if (is_rational) {
			if (ivd == 1) {
				val = js_new_i64_value(ctx, iv);
			}else {
				val = JS_NewArray(ctx);
				js_set_arr_size(ctx, val, 3);

				JS_SetPropertyUint32(ctx, val, 0, js_new_i64_value(ctx, iv));
				JS_SetPropertyUint32(ctx, val, 1, js_new_i64_value(ctx, ivd));
				JS_SetPropertyUint32(ctx, val, 2, JS_NewString(ctx, "/"));
			}
		}else {
			val = JS_NewFloat64(ctx, fv);
		}
	}
	else if (h.is_id()) {
		val = JS_NewInt32(ctx, 1);
	}else if (h.is_xempty()) {
		val = JS_NULL;
	}else if (create_vectors &&
		(h.tt == ETYPE::vector_imm || h.tt == ETYPE::vector)) 
	{
		let num_el = h.get_u24();
		val = JS_NewArray(ctx);
		js_set_arr_size(ctx, val, num_el);

		for (uint32_t i = 0; i < num_el; ++i) {
			operator_ptr pi;
			pi.a = &b;
			let offset = h.get_offset() + i;
			if (h.tt == ETYPE::vector_imm) {
				pi.h = h;
				pi.h.tt = ETYPE::number;
				pi.h.set_offset(offset);
			}else {
				pi.h = b.m_ops[offset].hdr;
			}
			auto el = create_js_value(ctx, pi, false);//recursion
			JS_SetPropertyUint32(ctx, val, i, el);//val[i]=el
		}
	}

	return val;
}

ims_static UNAMESPACE::unordered_set<const void*> g_rec_guard;

static const void* do_print(JSValue v)
{
	let* ptr = JS_VALUE_GET_PTR(v);
	assert(ptr);

	auto res = g_rec_guard.emplace(ptr);
	if (!res.second) {//already printed
		std::cout << "%";
		return nullptr;
	}

	return ptr;
}
static void js_print(JSContext* ctx, JSValue v)
{
	if (JS_IsArray(v)) {
		std::cout << "[";
		let* guard = do_print(v);
		if (guard) {
			let sz = js_get_arr_size(ctx, v);
			for (uint32_t i = 0; i < sz; ++i) {
				if (i > 0) {
					std::cout << ", ";
				}
				auto vi = JS_GetPropertyUint32(ctx, v, i);
				IMS_SCOPE([&] {JS_FreeValue(ctx, vi); });
				js_print(ctx, vi);//recursion
			}
			g_rec_guard.erase(guard);
		}
		std::cout << "]";
		return;
	}
	
	if (JS_IsObject(v)) {
		std::cout << "{";
		let* guard = do_print(v);
		if (guard) {
			uint32_t len = 0;
			JSPropertyEnum* ptab = nullptr;
			JS_GetOwnPropertyNames(ctx, &ptab, &len, v, JS_GPN_STRING_MASK);
			for (uint32_t i = 0; i < len; i++) {
				if (i > 0) {
					std::cout << ", ";
				}
				auto atom = ptab[i].atom;
				auto atomValue = JS_AtomToValue(ctx, atom);
				IMS_SCOPE([&] {JS_FreeValue(ctx, atomValue); });
				let* s1 = JS_ToCString(ctx, atomValue);
				IMS_SCOPE([&] {JS_FreeCString(ctx, s1); });
				std::cout << s1;
				std::cout << ": ";
				auto val = JS_GetProperty(ctx, v, atom);
				IMS_SCOPE([&] {JS_FreeValue(ctx, val); });
				js_print(ctx, val);//recursion
			}
			g_rec_guard.erase(guard);
		}
		std::cout << "}";
		return;
	}

	let* str = JS_ToCString(ctx, v);
	if (!str) {
		return;
	}
	std::cout << str;
	JS_FreeCString(ctx, str);
}

static JSValue js_console_log(
	JSContext* ctx,
	JSValueConst,
	int argc,
	JSValueConst* argv)
{
	for (int i = 0; i < argc; i++) {
		if (i > 0) {
			std::cout << " ";
		}
		g_rec_guard.clear();
		js_print(ctx, argv[i]);
	}
	std::cout << "\n";
	return JS_UNDEFINED;
}

static JSValue js_console_clear(
	JSContext* ,
	JSValueConst,
	int,
	JSValueConst*)
{
	void ext_console_clear();
	ext_console_clear();
	return JS_UNDEFINED;
}

static JSValue eval_module(
	JSContext* ctx,
	const std::string& filename,
	std::string_view src,
	std::string& err_str)
{
	JSValue mv = JS_Eval(ctx,
		src.data(),
		src.length(),
		filename.c_str(),
		JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);

	JSValue status = JS_EXCEPTION;

	if (!JS_IsException(mv)) {
		status = JS_EvalFunction(ctx, mv);
	}
	if (JS_IsException(status)) {
		JSValue e = JS_GetException(ctx);
		IMS_SCOPE([&] {JS_FreeValue(ctx, e); });
		js_get_error(e, ctx, err_str);
		return JS_EXCEPTION;
	}

	IMS_SCOPE([&] {JS_FreeValue(ctx, status); });
	assert(JS_IsObject(status));

	let state = JS_PromiseState(ctx, status);
	if (state == JS_PROMISE_REJECTED) {
		let e = JS_PromiseResult(ctx, status);	
		IMS_SCOPE([&] {JS_FreeValue(ctx, e); });
		js_get_error(e, ctx, err_str);
		return JS_EXCEPTION;
	}
	
	return JS_GetModuleNamespace(ctx, (JSModuleDef*)JS_VALUE_GET_PTR(mv));
}


js_engine::~js_engine()
{
	destroy();
}

////////////////////////////////////////////////////////////////////////////////

void js_engine::create()
{
	assert(!m_rt);

	m_rt = JS_NewRuntime();
	m_ctx = JS_NewContext(m_rt);

	m_jt = new js_aifs_block(m_ctx);

	//m_ctx = JS_NewContextRaw(m_rt);
	//JS_AddIntrinsicBaseObjects(m_ctx);
	//JS_AddIntrinsicBigInt(m_ctx);
	//JS_AddIntrinsicEval(m_ctx);
	//JS_AddIntrinsicDate(m_ctx);
	//JS_AddIntrinsicRegExpCompiler(m_ctx);
	//JS_AddIntrinsicRegExp(m_ctx);
	//JS_AddIntrinsicJSON(m_ctx);
	//JS_AddIntrinsicProxy(m_ctx);
	//JS_AddIntrinsicMapSet(m_ctx);
	//JS_AddIntrinsicTypedArrays(m_ctx);
	//JS_AddIntrinsicPromise(m_ctx);
	//JS_AddIntrinsicWeakRef(m_ctx);
	//JS_AddPerformance(m_ctx);

	JSValue global_obj = JS_GetGlobalObject(m_ctx);
	IMS_SCOPE([&] {JS_FreeValue(m_ctx, global_obj); });

	///////////////////////////////////////////////////////////////////////////

	JSValue console = JS_NewObject(m_ctx);
	JS_SetPropertyStr(m_ctx, global_obj, "console", console);
	JS_SetPropertyStr(m_ctx, console, "log",
		JS_NewCFunction(m_ctx, js_console_log, nullptr, 1));
	JS_SetPropertyStr(m_ctx, console, "clear",
		JS_NewCFunction(m_ctx, js_console_clear, nullptr, 0));
	

	JSValue ifs = JS_NewObject(m_ctx);
	JS_SetPropertyStr(m_ctx, global_obj, "ifs", ifs);//always present

	///////////////////////////////////////////////////////////////////////////
	for (auto& f : s_js_export) {
		f(m_ctx, global_obj);
	}
	///////////////////////////////////////////////////////////////////////////

	JS_SetInterruptHandler(m_rt, 
		[](JSRuntime*, void*){
			return ims_need_stop() ? 1 : 0;//executes quickly
		},
		nullptr);

	JS_SetModuleLoaderFunc(m_rt,
		nullptr,
		[](JSContext*, const char* module_name, void* je){
			return ((js_engine*)je)->module_loader_func(module_name);
		},
		this);
}


void js_engine::destroy()
{
	if (m_jt) {//before m_ctx
		delete m_jt;
		m_jt = nullptr;
	}

	if (m_ctx) {
		JS_FreeContext(m_ctx);
		m_ctx = nullptr;
	}

	if (m_rt) {
		JS_FreeRuntime(m_rt);
		m_rt = nullptr;
	}
}

void js_engine::thread_enter()
{
	JS_UpdateStackTop(m_rt);
}

void js_engine::eval(std::string_view src)
{
	if (!m_ctx) {
		create();//lazy creation
	} 
	
	std::scoped_lock lock(get_lock());
	thread_enter();
	
	auto v = JS_Eval2(m_ctx, src.data(), src.length(), nullptr);
	IMS_SCOPE([&] {JS_FreeValue(m_ctx, v); });

	if (JS_IsException(v)) {
		JSValue e = JS_GetException(m_ctx);
		IMS_SCOPE([&] {JS_FreeValue(m_ctx, e); });
		std::string err_str;
		js_get_error(e, m_ctx, err_str);
		std::cout << err_str;
	} else if (!JS_IsUndefined(v)){
		js_print(m_ctx, v);
	}
	
	std::cout << std::endl;
};


JSModuleDef* js_engine::module_loader_func(const char* module_name)
{
	if (module_name[0] == ':') {
		//TODO: loading from?
		return nullptr;
	}

	std::string contents;
	if (!ims_file::get_contents(module_name, contents)) {
		JS_ThrowReferenceError(m_ctx, "Could not load module filename '%s'",
			module_name);
		return nullptr;
	};

	//compile the module
	JSValue func_val = JS_Eval(m_ctx, 
		contents.c_str(), contents.size(), module_name,
		JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);

	if (JS_IsException(func_val)) {
		//TODO: print error
		return nullptr;
	}

	//the module is already referenced, so we must free it
	auto* m = (JSModuleDef*)JS_VALUE_GET_PTR(func_val);
	JS_FreeValue(m_ctx, func_val);
	return m;
}





////////////////////////////////////////////////////////////////////////////////



bool js_aifs_block::parse_var(
	size_t idx_var,//at depth it's always ims_max
	JSValue jv,
	size_t x,
	oper_block& b,
	read_state& rs,
	ims_identifiers& unk)
{
	
	auto& a = b.m_ops;
	a[x].hdr.clear();

	///////////////////////////////

	if (JS_IsNull(jv)) {//empty set
		a[x].hdr.set_xempty();
		return true;
	}

	if (JS_IsString(jv)) {

		std::string str;

		let* s = JS_ToCString(m_ctx, jv);
		str = s;
		JS_FreeCString(m_ctx, s);

		let err_msg = rs.parse_expression_as_aifs(unk, str, b, x);

		if (err_msg.empty()) {
			return true;
		}

		ims_error("{}", err_msg);
		return false;
		
	}

	if (JS_IsNumber(jv)) {
		int64_t res;
		if (get_int64(m_ctx, &res, jv)) {
			b.set_integer(x, res);
			return true;
		}

		double f64;
		if (0 == JS_ToFloat64(m_ctx, &f64, jv)) {
			b.set_double(x, f64);
			return true;
		}
		ims_error("Could not convert to a number");
		return false;
	}

	//below only an array is allowed
	let* it = m_enumerator.find(jv);
	if (!it) {
		ims_error("Array Expected");
		return false;
	}

	let& m = *it;

	//some arrays correspond to some named variable
	//in this case, we don't enter this block, but parse it
	//the first condition prevents the creation of definitions like s=s
	if (m.index != idx_var && m.index != ims_max) {
		let& var = rs.m_vars[m.index];
		a[x].hdr.set_reference(var.index7, 
			var.from_js? ESUBTYPE::ref_js: ESUBTYPE::ref_unknown);
		return true;
	}

	int64_t rat[2];
	if (get_rational64(m, rat)) {
		if (is_int32(rat[0]) && is_int32(rat[1])) {
			b.set_rational(x, (int32_t)rat[0], (int32_t)rat[1]);
			return true;
		}
	};

	ETYPE t = ETYPE::undef;

	size_t num_val = m.length;

	if (m.length > 0) {
		auto jlast = m_enumerator.m_values[m.start + m.length - 1];

		if (JS_IsString(jlast)) {
			let* s = JS_ToCString(m_ctx, jlast);

			switch (s[0]) {
			case '*':t = ETYPE::mul; break;
			case '+':t = ETYPE::sum; break;
			case '|':t = ETYPE::uni; break;
			case '^':t = ETYPE::power; break;
			case '/':t = ETYPE::inv; break;//conditionally
			}

			if (t != ETYPE::undef && s[1] != 0) {
				t = ETYPE::undef;
			}
			JS_FreeCString(m_ctx, s);

			if (t != ETYPE::undef) {
				--num_val;
			}
		}
	}


	if (t == ETYPE::power) {//a^b
		if (num_val != 2) {
			ims_error("Invalid number of arguments for ^");
			return false;
		}

		auto ar = b.set_power_ref(x);

		if (!parse_var(ims_max, m_enumerator.m_values[m.start], ar, b, rs, unk)) {
			return false;
		}
		if (!parse_var(ims_max, m_enumerator.m_values[m.start + 1], ar + 1, b, rs, unk)) {
			return false;
		}

		return true;
	}

	if (t == ETYPE::inv) {//a/b = a*inv(b)
		if (num_val != 2) {
			ims_error("Invalid number of arguments for /");
			return false;
		}

		auto ar = b.add_args(x, ETYPE::mul, 2);

		if (!parse_var(ims_max, m_enumerator.m_values[m.start], ar, b, rs, unk)) {
			return false;
		}

		let apos = b.add_args(ar + 1, ETYPE::inv, 1);

		if (!parse_var(ims_max, m_enumerator.m_values[m.start + 1], apos, b, rs, unk)) {
			return false;
		}

		return true;
	}


	if (t != ETYPE::undef) {
		auto ar = b.add_args(x, t, num_val);

		for (size_t i = 0; i < num_val; ++i) {
			auto ji = m_enumerator.m_values[m.start + i];
			if (!parse_var(ims_max, ji, ar++, b, rs, unk)) {
				return false;
			}
		}
		return true;
	}

	//vector
	auto ar = b.set_vector(x, num_val);

	for (size_t i = 0; i < num_val; ++i) {
		auto ji = m_enumerator.m_values[m.start + i];
		if (!parse_var(ims_max, ji, ar++, b, rs, unk)) {
			return false;
		}
	}

	b.adjust_vector(x);

	return true;
}

void process_description(JSContext* ctx, ims_identifiers& idf, oper_source& s, JSValue val)
{

	uint32_t len = 0;
	JSPropertyEnum* ptab = nullptr;
	JS_GetOwnPropertyNames(ctx, &ptab, &len, val, JS_GPN_STRING_MASK);
	for (uint32_t i = 0; i < len; i++) {
		auto atom = ptab[i].atom;

		auto jk = JS_AtomToValue(ctx, atom);
		IMS_SCOPE([&] {JS_FreeValue(ctx, jk); });

		let* k = JS_ToCString(ctx, jk);
		IMS_SCOPE([&] {JS_FreeCString(ctx, k); });
		if (!k)continue;

		auto jv = JS_GetProperty(ctx, val, atom);
		IMS_SCOPE([&] {JS_FreeValue(ctx, jv); });

		let* v = JS_ToCString(ctx, jv);
		IMS_SCOPE([&] {JS_FreeCString(ctx, v); });
		if (!v)continue;

		
		std::string tv = v;
		boost::algorithm::trim(tv);
		if (tv.empty())continue;

		std::string tk = k;
		boost::algorithm::trim(tk);

		if (tk.empty()) {
			s.md = tv;//description of the entire block
		}else {
			s.unk2description.emplace_back(
				idf.get_unk_id(tk), std::move(tv));
		}
	}
}



bool js_aifs_block::create_vars(
	oper_block& b,
	JSValue block_def,
	read_state& rs,
	ifs_list& lst)
{

	error_helper::line ehl(b.m_line8);

	assert(m_enumerator.empty());
	assert(m_js_vars.empty());
	IMS_SCOPE([&] {

		m_enumerator.clear(m_ctx);

		for (auto& e : m_js_vars) {
			JS_FreeValue(m_ctx, e);
		}
		m_js_vars.clear();
	});

	////////////////////////////////////////////////////////////////////////
	//parsing the block_def object

	uint32_t len = 0;

	JSPropertyEnum* ptab = nullptr;

	if (JS_GetOwnPropertyNames(m_ctx, &ptab, &len, block_def, JS_GPN_STRING_MASK)) {
		len = 0;//it's OK
	}

	rs.m_vars.clear();
	rs.m_vars.reserve(len);

	
	m_js_vars.reserve(len);




	for (uint32_t j = 0; j < len; j++) {

		let atom = ptab[j].atom;
		let val = JS_GetProperty(m_ctx, block_def, atom);
		IMS_SCOPE([&] {JS_FreeValue(m_ctx, val); });

		if (JS_IsUndefined(val)) {
			continue;
		}

		let atomValue = JS_AtomToValue(m_ctx, atom);
		IMS_SCOPE([&] {JS_FreeValue(m_ctx, atomValue); });
		let* s1 = JS_ToCString(m_ctx, atomValue);
		IMS_SCOPE([&] {JS_FreeCString(m_ctx, s1); });

		parsed_var v;
		v.line7 = rs.m_vars.size();
		v.name = s1;

		error_helper::var ehv(v.name);

		for (let c : v.name) {//TODO: proper validation is required
			if (!is_var_id_sym(c)) {
				ims_error("invalid name");
				return false;//critical error
			}
		}

		if (JS_IsString(val)) {
			let* s = JS_ToCString(m_ctx, val);
			v.val = s;
			JS_FreeCString(m_ctx, s);
		}

		if (v.name == ims_keywords::js_init) {//process it immediately
			assert(b.m_js_init == ims_max);
			if (JS_IsFunction(m_ctx, val)) {
				b.m_js_init = m_init_funcs.size();
				m_init_funcs.push_back({ JS_DupValue(m_ctx, val), ims_max });
			} else if (ims_identifiers::is_identifier(v.val)) {
				auto prop = get_exports_entry(v.val.c_str());
				IMS_SCOPE([&] {JS_FreeValue(m_ctx, prop); });
				if (!JS_IsFunction(m_ctx, prop)) {
					ims_error("$init must be an exported function.");
					return false;
				}
				b.m_js_init = m_init_funcs.size();
				m_init_funcs.push_back({ JS_DupValue(m_ctx, prop), lst.m_idf.get_unk_id(v.val) });
			} else {
				ims_error("$init must be a function or identifier.");
				return false;
			}
			v.val.clear();//consumed
		}else  if (!v.val.empty()) {
			//parse it later as AIFS
		} else if (v.name == ims_keywords::dim) {//process it immediately
			int64_t d;
			if (get_int64(m_ctx, &d, val)) {
				if (d > 100) {
					ims_error("must be between 0 and 100");
					return false;
				}
				b.m_flags.has_dim = true;
				b.m_dim2 = (size_t)d;
			}
			else {
				ims_error("invalid value");
				return false;
			}
		}else if (v.name == ims_keywords::js_info) {//process it immediately
			if (JS_IsObject(val)) {
				auto& s = b.m_src2;
				if (!s)s.reset(new oper_source);
				process_description(m_ctx, lst.m_idf, *s, val);
			}else {
				ims_warning("must be an object");
			}
		} else {

		}
		v.js_val = m_js_vars.size();
		m_js_vars.emplace_back(JS_DupValue(m_ctx, val));

		rs.m_vars.emplace_back(v);

		m_enumerator.process_arrays(val, m_ctx);
	}


	size_t line_name = ims_max;
	size_t line_attr = ims_max;

	if (!rs.process_string_vars(lst, b, line_name, line_attr)) {
		return false;
	}

	/////////////////////////////////////////////////////////////
	//create additional variables
	len = (uint32_t)rs.m_vars.size();
	size_t next_var = len;
	for (uint32_t j = 0; j < len; j++) {
		let& v = rs.m_vars[j];

		if (v.js_val == ims_max) {
			continue;
		}
		let jv = m_js_vars[v.js_val];

		if (!JS_IsArray(jv)) {
			continue;
		}

		auto* it = m_enumerator.find(jv);
		if (!it) {
			assert(false);//interesting
			continue;
		}
		if (it->index == ims_max) {
			it->index = j;
		}
	}

	let num_new_vars = m_enumerator.set_index(next_var) - next_var;
	rs.m_vars.resize(rs.m_vars.size() + num_new_vars);

	for (let& jv : m_enumerator.m_values) {
		if (!JS_IsArray(jv)) {
			continue;
		}
		let* it = m_enumerator.find(jv);
		if (!it ||
			it->index == ims_max ||
			it->index < len)
		{
			continue;
		}
		auto& var = rs.m_vars[it->index];

		var.js_val = m_js_vars.size();
		m_js_vars.emplace_back(JS_DupValue(m_ctx, jv));

		var.line7 = it->index;
	}

	//////////////////////////////////////////////////////////////
	//final processing

	//unnamed indexes
	for (uint32_t j = len; j < rs.m_vars.size(); j++) {
		auto& vj = rs.m_vars[j];
		vj.index7 = j - len;
		vj.from_js = true;
	}

	//all indexes are ready
	for (uint32_t j = 0; j < rs.m_vars.size(); j++) {

		auto& q = rs.m_vars[j];

		if (q.index7 == ims_max || !q.val.empty()) {
			continue;//processed as AIFS
		}

		let offset = b.add_var(rs.m_pfo.m_pos2, q.index7, q.is_subst);

		if (q.js_val == ims_max) {
			continue;
		}
		let jv = m_js_vars[q.js_val];

		error_helper::var ehv(q.name);

		if (!parse_var(j, jv, offset, b, rs, lst.m_idf)) {
			b.m_ops[offset].hdr.is_xundef();
			return false;
		}

		//last iteration over named
		if (j + 1 == len) {//let's count how many regular variables were added
			size_t num = 0;
			for (let&x : b) { ++num; x; };
			b.m_named_vars = num;
		}
	}

	return true;
}

size_t js_engine::add_js_init(size_t unk_id, const char* fname)
{
	if (!m_ctx) {
		create();//lazy creation
	}

	auto prop = m_jt->get_exports_entry(fname);
	IMS_SCOPE([&] {JS_FreeValue(m_ctx, prop); });

	if (!JS_IsFunction(m_ctx, prop)) {
		return ims_max;
	}
	let ret = m_jt->m_init_funcs.size();
	m_jt->m_init_funcs.push_back({ JS_DupValue(m_ctx, prop), unk_id });
	return ret;
}

size_t js_engine::get_js_init_identifier(size_t idx) const
{
	return m_jt->m_init_funcs[idx].unk_id;
}

static JSValue create_js_value(JSContext* ctx, const ims_val* v)
{
	if (!v)return JS_UNDEFINED;
	if (v->is(ims_val_b::ETP::string)) {
		let str = v->get_string();
		return JS_NewStringLen(ctx, str.data(), str.size());
	}
	if (v->is(ims_val_b::ETP::number, ims_val_b::EST::rational)) {
		if (v->p_i()->denominator() == 1) {
			return JS_NewInt64(ctx, v->p_i()->numerator());
		}
	} else if (v->is(ims_val_b::ETP::number, ims_val_b::EST::big_rational)) {
		if (denominator(*v->p_b()) == 1) {
			auto str = numerator(*v->p_b()).str();
			str += "n";
			return JS_Eval2(ctx, str.data(), str.length(), nullptr);
		}
	}
	double dv;
	if (v->to_real(dv)) {
		return JS_NewFloat64(ctx, dv);
	}
	if (!v->is(ims_val_b::ETP::vector, ims_val_b::EST::other)) {
		return JS_UNDEFINED;
	}

	let sz = v->get_size();
	JSValue arr = JS_NewArray(ctx);
	js_set_arr_size(ctx, arr, sz);
	for (size_t i = 0; i < sz; ++i) {
		JS_SetPropertyUint32(ctx, arr, (uint32_t)i, create_js_value(ctx, v->p_v()[i]));
	}
	return arr;
}

std::string js_engine::create_from_constructor(const ims_val* v,
	read_state& rs,
	ifs_list& lst)
{
	std::scoped_lock lock(js_engine::get_lock());
	thread_enter();

	assert(!JS_IsUndefined(m_jt->m_constructor));

	//call JS
	JSValue obj = create_js_value(m_ctx, v);
	IMS_SCOPE([&] {JS_FreeValue(m_ctx, obj); });

	auto aifs = JS_Call(m_ctx, m_jt->m_constructor, obj, 0, nullptr);
	IMS_SCOPE([&] {JS_FreeValue(m_ctx, aifs); });

	if (JS_IsException(aifs)) {
		std::string err_str;
		JSValue e = JS_GetException(m_ctx);
		IMS_SCOPE([&] {JS_FreeValue(m_ctx, e); });
		js_get_error(e, m_ctx, err_str);
		std::string ret;
		fmt::format_to(std::back_inserter(ret), "JS constructor error: {}", err_str);
		return ret;
	}

	m_jt->create_blocks(aifs, rs, lst);

	return "";
}


bool js_aifs_block::add_block3(
	JSValue block_def,
	read_state& rs,
	ifs_list& lst)
{
	JSValue cur = block_def;

	oper_block* last_parent = nullptr;
	std::string last_par_id;

	
	assert(m_parr.empty());
	IMS_SCOPE([&] {m_parr.clear(); });

	for (;;) {
		if (!JS_IsObject(cur)) {
			ims_error("Block must be an object");
			return false;
		}

		auto res = m_blocks2.emplace(JS_VALUE_GET_PTR(cur), nullptr);
		if (!res.second) {
			last_parent = res.first->second;//already inserted
			if (!last_parent) {
				ims_error("Recursive parent detected");
				return false;
			}
			if (m_parr.empty()) {
				return true;//inserted and has no descendants
			}
			break;
		}

		auto& nx = m_parr.emplace_back();
		nx.ppb = &res.first->second;
		nx.v = cur;

		JSValue val_parent = JS_GetPropertyStr(m_ctx, cur, ims_keywords::parent);
		IMS_SCOPE([&] {JS_FreeValue(m_ctx, val_parent); });

		if (JS_IsUndefined(val_parent)) {
			break;
		}

		if (JS_IsString(val_parent)) {
			let* s0 = JS_ToCString(m_ctx, val_parent);
			last_par_id = s0;
			JS_FreeCString(m_ctx, s0);
			break;
		}

		cur = val_parent;
	}

	//3 options: last_parent, last_par_id, and nothing

	//TODO: Do we need to add it in a special order?

	auto parent_idx = block_id_max;

	if (!last_par_id.empty()) {
		parent_idx = lst.insert_by_str_id(last_par_id);
	}
	
	if (last_parent) {
		if (last_parent->m_block_id == block_id_max) {
			last_par_id = lst.m_idf.gen_unique_block_id("UN");
			last_parent->m_block_id = lst.insert_by_str_id(last_par_id);
			auto* q = lst.get_block(last_parent->m_block_id);

			//TODO: maybe everything is okay now?
			assert(false);
			//q->b = last_parent;
		}
		parent_idx = last_parent->m_block_id;
	}

	boost::algorithm::trim(last_par_id);

	for (auto& jb : boost::adaptors::reverse(m_parr)) {
	
		JSValue val_str_id = JS_GetPropertyStr(m_ctx, jb.v, ims_keywords::str_id);
		IMS_SCOPE([&] {JS_FreeValue(m_ctx, val_str_id); });
		std::string str_id;
		if (!JS_IsUndefined(val_str_id)) {
			if (!JS_IsString(val_str_id)) {
				ims_error("Invalid ID");
				return false;
			}
			let* s0 = JS_ToCString(m_ctx, val_str_id);
			str_id = s0;
			JS_FreeCString(m_ctx, s0);

			boost::algorithm::trim(str_id);

			if (lst.find_block(str_id)) {
				//warning
				WARN_DUP_ID(str_id);
				return true;
			};
		}

		auto& b = *lst.add_block(str_id);

		

		//TODO: block index in the array
		b.m_line8 = 1;

		b.m_flags.from_js = true;
		*jb.ppb = &b;//filling the m_blocks hash table
		
		if (last_parent) {
			b.set_parent(last_parent);
		}
		if (parent_idx != block_id_max) {
			b.m_parent_id = parent_idx;
		}

		if (!create_vars(b, jb.v, rs, lst)) {
			return false;
		}

		last_parent = &b;
		parent_idx = b.m_block_id;
	}

	return true;
}


static void js_reg_cur_module(JSContext* ctx, JSValue module_export)
{
	JSValue global_obj = JS_GetGlobalObject(ctx);
	IMS_SCOPE([&] {JS_FreeValue(ctx, global_obj); });

	auto ifs_obj = JS_GetPropertyStr(ctx, global_obj, "ifs");
	IMS_SCOPE([&] {JS_FreeValue(ctx, ifs_obj); });

	JS_SetPropertyStr(ctx, ifs_obj, "m",JS_DupValue(ctx, module_export));
};

bool js_engine::get_blocks_from_js(
	const std::string& filename,
	std::string_view src,
	std::string& description,
	read_state& rs,
	ifs_list& lst,
	pool_ptr& constructor_dialog)
{
	description.clear();

	assert(!src.empty());

	if (!m_ctx) {
		create();//lazy creation
	}

	m_jt->clear9();//full cleaning


	std::scoped_lock lock(js_engine::get_lock());
	thread_enter();


	//JS execution
	std::string err_string;
	JSValue exports = eval_module(m_ctx, filename, src, err_string);
	IMS_SCOPE([&] {JS_FreeValue(m_ctx, exports); });
	
	if (JS_IsException(exports)) {
		ims_error("JS exception: {}", err_string);
		return false;//critical error
	}

	m_jt->m_export = JS_DupValue(m_ctx, exports);
	if (!m_jt->init_constructor(constructor_dialog)) {
		return false;//critical error
	};

	js_reg_cur_module(m_ctx, exports);

	auto desc = JS_GetPropertyStr(m_ctx, exports, ims_keywords::js_info);
	IMS_SCOPE([&] {JS_FreeValue(m_ctx, desc); });
	if (!JS_IsUndefined(desc)) {
		if (!JS_IsString(desc)) {
			ims_warning("Description is not loaded, invalid type.");
		}else {
			let* str = JS_ToCString(m_ctx, desc);
			if (str)description = str;
			JS_FreeCString(m_ctx, str);
		}
	}

	auto aifs = JS_GetPropertyStr(m_ctx, exports, ims_keywords::js_export_blocks);
	IMS_SCOPE([&] {JS_FreeValue(m_ctx, aifs); });

	return m_jt->create_blocks(aifs, rs, lst);
}

////////////////////////////////////////////////////////////////////////////////
#include "ims_info.h"
#include "block_class.h"
#include "ims_view.h"

ims_static read_state g_rs;//TODO get rid of

//call $init
//returns false on error
//called only from check_block_ex
bool call_js_init(oper_block& b, size_t idx, ims_view<operator_ptr> ovr)
{
	auto& nfo = const_cast<ims_info&>(*b.get_class()->m_nfo);
	auto& je = nfo.m_js_engine;

	std::scoped_lock lock(je.get_lock());
	je.thread_enter();

	auto* jtx = je.get_ctx();

	JSValue obj = JS_NewObject(jtx);
	IMS_SCOPE([&] {JS_FreeValue(jtx, obj); });

	std::string var_name;

	let* g = b.get_class();

	let sz = b.num_vars();
	for (size_t i = 0; i < sz; ++i) {
		if (ovr[i].h.is_xundef())continue;
		let val = create_js_value(jtx, ovr[i], true);
		if (JS_IsUndefined(val)) {
			return false;
		}
		var_name = g->get_var_name(i);
		JS_SetPropertyStr(jtx, obj, var_name.c_str(), val);
	}

	//call JS
	let status = JS_Call(jtx, je.m_jt->m_init_funcs[idx].v, obj, 0, nullptr);
	IMS_SCOPE([&] {JS_FreeValue(jtx, status); });

	if (JS_IsException(status)) {
		std::string err_str;
		JSValue e = JS_GetException(jtx);
		IMS_SCOPE([&] {JS_FreeValue(jtx, e); });
		js_get_error(e, jtx, err_str);
		ims_error("JS exception : {}", err_str);
		return false;
	}

	//remove fields that are not allowed to be changed
	for (let& q : b) {
		if (q.is_builtin())continue;
		var_name = g->get_var_name(q.gr());
		JS_SetPropertyStr(jtx, obj, var_name.c_str(), JS_UNDEFINED);
	}

	IMS_SCOPE([] { g_rs.clear(); });
	
	error_helper::line ehl(b.m_line8);

	b.prepare_js_parent();
	return je.m_jt->create_vars(*b.m_js_parent, obj, g_rs, nfo.m_list);
}

