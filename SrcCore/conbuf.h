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


template<bool IsErr>
struct conbuf_t : public std::stringbuf
{
public:

	~conbuf_t() {/*sync();*/revert(); }

	void redirect()
	{
		m_old = get_global_stream().rdbuf(this);	
	}

	void revert()
	{
		if (!m_old)return;
		get_global_stream().rdbuf(m_old);
		m_old = nullptr;
	}

	std::string fetch_data(std::recursive_mutex& lock)
	{
		std::string ret;
		{
			std::scoped_lock _(lock);
			ret = this->str();
			this->str("");
		}
		return ret;
	}
	
protected:
	int sync() override
	{
		
#if 0
		if (!m_allow_sync)return 0;

		auto s = fetch_data();
		if (s.empty())return 0;

		void ext_console_out(const char* str);
		ext_console_out(s.c_str());
#endif
		return 0;
	};

	

private:

	static std::ostream& get_global_stream() {
		if constexpr (IsErr) {
			return std::cerr;
		} else {
			return std::cout;
		}
	}

	typename std::streambuf* m_old = nullptr;
};

////////////////////////////////////////////////////////////////////////////////

struct console_writer
{
public:	
	conbuf_t<false>	m_out_buf;
	conbuf_t<true>	m_err_buf;

	//std::stringstream m_ss_msg;
	//std::stringstream m_ss_err;

	std::recursive_mutex m_lock;

	void redirect();

	void revert();

	std::string fetch_error();
	std::string fetch_string();
};
