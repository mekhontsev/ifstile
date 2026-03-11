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

#pragma once

struct JSRuntime;
struct JSContext;
struct JSModuleDef;
struct JSValue;

struct ifs_list;
struct read_state;
struct js_aifs_block;
struct ims_val;
struct pool_ptr;

struct js_engine: public boost::noncopyable
{
	js_engine() = default;
	~js_engine();


	void create();
	void destroy();

	JSContext* get_ctx() { return m_ctx; };

	[[nodiscard]]
	bool get_blocks_from_js(
		const std::string& filename,
		std::string_view src, 
		std::string& description,//output parameter
		read_state& rs, 
		ifs_list& lst,
		pool_ptr& constructor_dialog);

	//called when the thread changes
	void thread_enter();
	
	std::mutex& get_lock() { return m_lock; };

	js_aifs_block* m_jt = nullptr;

	void eval(std::string_view src);

	using reg_function = void (*)(JSContext*, JSValue&);
	static std::vector<reg_function> s_js_ifs_object;

	size_t js_obj_get_identifier(size_t idx) const;
	const ims_val* js_obj_get_imm_value(size_t idx);
	bool js_obj_is_function(size_t idx) const;
	const ims_val* js_obj_call(
		size_t idx, const ims_val* arg, size_t argc,  std::string& err);

	std::string  create_from_constructor(
		const ims_val* v,
		read_state& rs,
		ifs_list& lst);

	static void js_set_arr_size(JSContext* ctx, JSValue& arr, size_t size);
	static int64_t js_get_arr_size(JSContext* ctx, const JSValue& v);;
	static bool get_int64(JSContext* ctx, int64_t* pres, const JSValue& val);


private:

	std::mutex m_lock;

	JSRuntime* m_rt = nullptr;
	JSContext* m_ctx = nullptr;
	JSModuleDef* module_loader_func(const char* module_name);
};

