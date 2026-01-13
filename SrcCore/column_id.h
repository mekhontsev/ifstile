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

struct column_id
{
	enum ECID : size_t
	{
		CH,
		HD,
		Name,
		Graph,
		ID,
		GCX,
		NGV,
		NGE,
		NGC,
		CT,
		NDIM2,
		DIM2,
		INFM2,
		DIM3,
		INFM3,
		Z2,
		Z3,
		//	CT3,
		SDIM2,
		SM2,
		LDIM2,
		DPT,
		FLI,
		FLO,
		NBH,
		DIM,
		DA,
		DP,
		SIM,
		ORI,
		RF,
		RM,		
		SID,
		MID,
		NMID,
		BISO,
		AR2,
		R2,
		NR,
		OVL,
		OVF,
		ARTH,
		BITS,
		PRC,
		MUT,
		SMUT,
		CM,
		PRT,
		GEN,
		TIME,

#if defined(DEVELOPER_VERSION)
		_LDIM2,
		_R2,
#endif
		NUM_COLS,
	};
};