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


//calling a function in the context of the main thread
/*
main_thread::post([]()
{
	...
});
*/

void ext_async_message(void* p);

struct main_thread
{
	//without waiting
	template<typename T>
	static void post(T& t)
	{
		struct ctex: public main_thread
		{
			T m_t;//value
			ctex(T& t) :m_t(std::move(t)) {};
			void call() override { m_t(); };
		};
		ext_async_message(new ctex(t));
	}

	//with waiting
	template<typename T>
	static void send(T& t)
	{
		bool ready = false;

		struct ctex: public main_thread
		{
			const T& m_t;//reference
			bool& m_ready;
			ctex(const T& t, bool& b):m_t(t), m_ready(b) {}
			void call() override { m_t(); m_ready = true; };
		};
		ext_async_message(new ctex(t,ready));

		//spin-lock
		while (!ready) {
			std::this_thread::yield();
		}
	}


	static void dispatch(void* p)
	{
		std::unique_ptr<main_thread> q{ static_cast<main_thread*>(p) };
		if (q)q->call();	
	}

	virtual ~main_thread() = default;
	
private:
		
	virtual void call()=0;
};
	
