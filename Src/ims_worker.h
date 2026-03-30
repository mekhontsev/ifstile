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
#include "ims_chrono.h"

struct ims_stage;

struct ims_worker : public boost::noncopyable
{
	//must be called once from the main thread
	static void init_main();

	static ims_worker* get();

	static bool is_main_thread();

	//number of running threads
	static int active_workers();

	static void exit_program();
	static bool is_exit_program();


	static void pause_workers(bool p);

	static void create_threads(size_t num);
	static void destroy_threads();
	static void stop_all();
	static ims_worker& get_thread(size_t idx);

	////////////////////////////////////////////////////////////////////////////

	enum class status
	{
		stop,
		pause,
		work,
	};

	void thread_proc();
	void init_thread();

	void deinit_thread();

	bool is_running() const;


	int64_t running_time_ms() const;

	using FUNC = std::function<void()>;

	void start(FUNC&& f);

	//request from the main thread to stop
	//min_time_ms - let the thread run
	void stop(uint64_t min_time_ms = 0);

	//pause or unpause
	void pause(bool p);

	//under normal conditions (m_req_status == work) it works fast
	bool is_need_stop2();

	////////////////////////////////////////////////////////////////////////////

	//start time
	ims_chrono m_time_start;

	ims_stage* m_stage = nullptr;

	void try_to_continue() { m_req_status = status::work; };
private:

	bool m_idle = true;

	status m_req_status = status::stop;

	//if stop is requested, then not earlier than this value
	ims_chrono m_time_stop;

	//thread procedure
	FUNC m_func;

	//freed only when there is nothing to do
	mutable std::mutex m_lock;
	std::condition_variable m_cond;
	std::thread m_cur_thread;
};