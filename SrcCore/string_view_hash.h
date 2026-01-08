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


struct string_view_hash_dense {
	using is_transparent = void; // enable heterogeneous overloads
	//  using is_avalanching = void; // mark class as high quality avalanching hash

	[[nodiscard]] auto operator()(std::string_view str) const noexcept -> uint64_t {
		return ankerl::unordered_dense::hash<std::string_view>{}(str);
	}
};


struct string_view_hash
{
	using is_transparent = void; // Enables heterogeneous lookup

	std::size_t operator()(const std::string& s) const { return std::hash<std::string>{}(s); }
	std::size_t operator()(std::string_view sv) const { return std::hash<std::string_view>{}(sv); }
};

