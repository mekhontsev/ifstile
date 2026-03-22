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

struct ims_chrono 
{
	using HRC = std::chrono::high_resolution_clock;
	using MS = std::chrono::milliseconds;

	static ims_chrono now() {
		return ims_chrono{ HRC::now() };
	};

	static int64_t dif_ms(const ims_chrono& t1, const ims_chrono& t2)
	{
		return std::chrono::duration_cast<MS>(t2.m_t - t1.m_t).count();
	}

	static int64_t dif_micro(const ims_chrono& t1, const ims_chrono& t2)
	{
		return std::chrono::duration_cast<std::chrono::microseconds>(t2.m_t - t1.m_t).count();
	}

	int64_t to_now_micro() const
	{
		return dif_micro(*this, now());
	}

	int64_t to_now_ms() const
	{
		return dif_ms(*this, now());
	}

	static int64_t ms_since_epoch()
	{
		return std::chrono::duration_cast<MS>(
			std::chrono::system_clock::now().time_since_epoch()).count();
	}

	struct fmt_ms_to_hour_minutes_seconds
	{
		fmt_ms_to_hour_minutes_seconds(uint64_t ms)
		{
			let time_elapsed = double(ms) / 1000;

			auto n = uint64_t(time_elapsed);
			let hour = n / 3600; 
			n %= 3600;
			let minutes = n / 60;
			let seconds = n % 60;

			fmt::format_to_n(buf.data(), buf.size(), 
				"{}:{:02}:{:02}\0", hour, minutes, seconds);
		};
		std::array<char, 32> buf = {};
	};


	struct fmt_time_t_to_hmsdmY
	{
		fmt_time_t_to_hmsdmY(): fmt_time_t_to_hmsdmY(std::time(nullptr)) {};

		fmt_time_t_to_hmsdmY(time_t tm)
		{
			struct tm t;
#if defined(_WIN32) || defined(_WIN64)
			localtime_s(&t, &tm);
#else
			localtime_r(&tm, &t);
#endif
			let res = std::strftime(buf.data(), buf.size(),
				"%H:%M:%S %d-%m-%Y", &t);
			if(!res) buf[0] = 0;
		}
		std::array<char, 32> buf = {};
	};

	[[nodiscard]]
	ims_chrono add_ms(int64_t ms) const
	{
		return ims_chrono{ m_t + MS(ms) };
	}

	HRC::time_point m_t;
};
