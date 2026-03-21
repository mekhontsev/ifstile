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
#include "ui_window.h"

struct ws_modal_msg : public window_state
{
	const char* get_title() override;
	
	static void show_ext(
		const char* title,
		std::string_view text, 
		bool* p_ok, 
		bool* p_cancel);

	void show()  override;
	void confirm(std::string_view msg, std::function<void()>&& F);
	void message(std::string_view msg);
	bool need_to_show() const;

	bool close();
private:

	std::string m_message;//current modal message
	std::function<void(void)> m_confirm;//procedure for confirmation message

};
