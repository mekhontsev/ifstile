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

struct ims_ini 
{
	bool read(std::istream& fs);
	void write(std::ostream& fs) const;

	void puts(const std::string& key, std::string_view v) 
	{
		auto& s = m_data[key];
		s = v;
		boost::algorithm::trim(s);
	};

	template<typename T>
	void put(const std::string& key, const T& v)
	{
		if constexpr (std::is_same<T, bool>::value) {
			m_data[key] = v ? "true" : "false";
		} else {
			m_data[key] = std::to_string(v);
		}
		
	}
	

	template<typename T>
	bool get(const std::string& key, T& v) const
	{
		let it = m_data.find(key);
		if (it != m_data.end()) {
			let& s = it->second;
			if constexpr (std::is_same<T, bool>::value) {
				if (s == "true") {
					v = true;
					return true;
				}
				if (s == "false") {
					v = false;
					return true;
				}
			} else {
				if (boost::conversion::try_lexical_convert(s, v)) {
					return true;
				}
			}
		}
		return false;
	}


	
private:

	UNAMESPACE::unordered_map<std::string, std::string> m_data;
};