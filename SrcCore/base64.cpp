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
#include "base64.h"




static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
								'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
								'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
								'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
								'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
								'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
								'w', 'x', 'y', 'z', '0', '1', '2', '3',
								'4', '5', '6', '7', '8', '9', '-', '_' };



static bool is_base64(uint8_t c) {
	return (isalnum(c) || (c == '-') || (c == '_'));
}

bool is_base64(std::string_view s)
{
	//there can be up to two '=' at the end
	if (!s.empty() && s.back() == '=')s.remove_suffix(1);
	if (!s.empty() && s.back() == '=')s.remove_suffix(1);

	for (let c : s) {
		if (!is_base64((uint8_t)c)) {
			return false;
		}
	}

	return true;
}



void base64_encode(const char* data, size_t input_length, std::string& dst)
{
	assert(data != dst.data());
	size_t output_length = 4 * ((input_length + 2) / 3);

	dst.resize(output_length);

	char* encoded_data = dst.data();
	
	for (size_t i = 0, j = 0; i < input_length;) {

		uint32_t octet_a = i < input_length ? (uint8_t)data[i++] : 0;
		uint32_t octet_b = i < input_length ? (uint8_t)data[i++] : 0;
		uint32_t octet_c = i < input_length ? (uint8_t)data[i++] : 0;

		uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

		encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
		encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
		encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
		encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
	}

	static size_t mod_table[3] = { 0, 2, 1 };

	for (size_t i = 0; i < mod_table[input_length % 3]; i++) {
		encoded_data[output_length - 1 - i] = '=';
	}
}


bool base64_decode(const char* data, size_t input_length, std::string& dst) 
{
	assert(data != dst.data());

	static std::vector<char> decoding_table;

	if (decoding_table.empty()) {
		decoding_table.resize(256);

		for (uint8_t i = 0; i < 64; i++) {
			decoding_table[(uint8_t)encoding_table[i]] = i;
		}
	}

	if (input_length % 4 != 0) return false;

	size_t output_length = input_length / 4 * 3;
	if (data[input_length - 1] == '=') output_length--;
	if (data[input_length - 2] == '=') output_length--;

	dst.resize(output_length);

	char* decoded_data = dst.data();
	
	for (size_t i = 0, j = 0; i < input_length;) {

		uint32_t sextet_a = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
		uint32_t sextet_b = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
		uint32_t sextet_c = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
		uint32_t sextet_d = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];

		uint32_t triple = (sextet_a << 3 * 6)
			+ (sextet_b << 2 * 6)
			+ (sextet_c << 1 * 6)
			+ (sextet_d << 0 * 6);

		if (j < output_length) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
		if (j < output_length) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
		if (j < output_length) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
	}

	return true;
}




