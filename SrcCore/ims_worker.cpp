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

#include "pch.h"
#include "ims_worker.h"


ims_static thread_local ims_worker* s_nfo = nullptr;

ims_static ims_random g_rng;//for the main thread

//number of running threads
ims_static std::atomic<int> g_num_workers{};

ims_static bool s_do_exit_program = false;

ims_static std::vector<std::unique_ptr<ims_worker>> g_threads;

////////////////////////////////////////////////////////////////////////////////


void ims_worker::init_main()
{
	s_nfo = nullptr;
	g_rng.seed();
}

ims_worker* ims_worker::get()
{
	return s_nfo;
}


int ims_worker::active_workers()
{
	return g_num_workers;
};


void ims_worker::exit_program()
{
	s_do_exit_program = true;
}

bool ims_worker::is_exit_program()
{
	return s_do_exit_program;
}

void ims_worker::create_threads(size_t num)
{
	g_threads.resize(num);
	for (auto& t : g_threads) {
		if (t)continue;
		t.reset(new ims_worker);
		t->init_thread();
	}
}

void ims_worker::destroy_threads()
{
	for (auto& t : g_threads) {
		t->deinit_thread();
	}
	g_threads.clear();
}

void ims_worker::stop_all()
{
	for (auto& t : g_threads) {
		t->stop();
	}
}

ims_worker& ims_worker::get_thread(size_t idx)
{
	return *g_threads[idx];
}

bool ims_worker::is_main_thread()
{
	return get() == nullptr;
}

ims_random& ims_worker::rng()
{
	auto* p = get();
	return p ? p->m_rng : g_rng;
};

void ims_worker::pause_workers(bool p)
{
	for (auto& t : g_threads) {
		t->pause(p);
	}
}

bool ims_need_stop()
{
	auto* p = ims_worker::get();
	if (!p)return false;//main thread
	return p->is_need_stop2();	
}

////////////////////////////////////////////////////////////////////////////////


void ims_worker::thread_proc()
{
	
	assert(!s_nfo);
	s_nfo = this;

	while (!s_do_exit_program) {

		{
			while (!m_func || m_req_status != status::work) {
				std::unique_lock<std::mutex> lk(m_lock);
				m_idle = true;
				m_cond.wait(lk);//may wake up sporadically
				if (s_do_exit_program)return;
			}
			m_idle = false;
		}
		

		++g_num_workers;

		m_time_start = ims_chrono::now();

		try {
			m_func();//thread procedure
		}
		catch (const std::exception& e) {
			ims_error("Error: {}", e.what());
		}

		{
			std::scoped_lock lk(m_lock);
			m_func = nullptr;
		}
		
		--g_num_workers;
	};
};

void ims_worker::init_thread() 
{
	m_rng.seed();
	m_cur_thread = std::thread(&ims_worker::thread_proc,this);
}


void ims_worker::deinit_thread()
{
	assert(s_do_exit_program);
	stop();
	m_cond.notify_one();
	m_cur_thread.join();
};


bool ims_worker::is_running() const
{
	return !m_idle;
}

int64_t ims_worker::running_time_ms() const
{
	return m_idle ? 0 : m_time_start.to_now_ms();
}

bool ims_worker::is_need_stop2()
{
	assert(!is_main_thread());//worker thread checks itself

	if (m_req_status == status::pause){
		--g_num_workers;

		while (m_req_status == status::pause && !s_do_exit_program) {
			std::unique_lock<std::mutex> lk(m_lock);
			m_cond.wait(lk);
		}
		++g_num_workers;
	}
	
	return s_do_exit_program ||
		(m_req_status == status::stop && ims_chrono::now().m_t >= m_time_stop.m_t);
}


void ims_worker::start(FUNC&& f)
{
	assert(!is_running());
	assert(is_main_thread());//running is allowed only from the main thread

	work_reset();
	
	{
		std::scoped_lock lk(m_lock);
		m_func = std::move(f);
	}
	
	m_req_status = status::work;
	m_cond.notify_one();
}


void ims_worker::pause(bool p)
{
	assert(is_main_thread());
	if (m_req_status == status::stop)return;
	m_req_status = p ? status::pause : status::work;
	if (!p) m_cond.notify_one();
}


void ims_worker::stop(uint64_t min_time_ms)
{
	assert(is_main_thread());
	
	if (m_req_status != status::stop) {
		m_time_stop = m_time_start.add_ms(min_time_ms);
		m_req_status = status::stop;
	}
	
	m_cond.notify_one();//in case the thread was paused
}

void ims_worker::work_reset()
{
	m_work_done = 0;
}


