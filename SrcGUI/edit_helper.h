#pragma once

namespace edit_helper
{
	enum
	{
		a_none,
		a_cut,
		a_copy,
		a_paste,
		a_undo,
		a_redo,
		a_selall,
	};

	void set_action(int id, int a);
	void begin(int id);
	void end(bool read_only = false);
};
