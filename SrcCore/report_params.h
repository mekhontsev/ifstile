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

struct report_params
{
	report_params(): 
		only_max_inters(true),
		intersections(false), 
		connections(false), 
		neighbourhoods(true),
		neighbourhoods_graph(false),
		nboundary(false),
		relators(false),	
		filer(filter_type::all),
		filer_post(filter_type::all)
	{};

	enum class filter_type : uint8_t
	{
		all,
		positive,
		max
	};


	//maximum intersections to show
	size_t max_inter = 2;

	size_t max_complexity=1000;
	size_t max_bits=63;
	float find_prec2=0;//0.3 is a reasonable value

	bool only_max_inters : 1;		//show only maximum intersections
	bool only_strong_intres : 1;	//only edges leading to the same component
	bool intersections : 1;			//parts intersections
	bool connections : 1;			//chunk junctions
	bool neighbourhoods : 1;
	bool neighbourhoods_graph : 1;
	bool nboundary : 1;				//neighborhood-based boundary
	bool relators : 1;				//group relators
	

	filter_type filer : 2;
	filter_type filer_post : 2;

	bool empty() const
	{
		return !intersections && !connections &&
			!neighbourhoods && !neighbourhoods_graph && !nboundary && !relators;
	}

};
