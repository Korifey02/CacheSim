#include <cstring>
#include "globals.h"

int my_strcpy_s(char* dst, size_t dstsz, const char* src) {
	if (!dst || !src || dstsz == 0) return -1;
	size_t len = std::strlen(src);
	if (len >= dstsz) return -1;
	std::memcpy(dst, src, len + 1);
	return 0;
}

std::uint32_t find_index(struct array_type_sim* array, char* str, int32_t max_num)
{
	for (int i = 0; i < max_num; i++)
		if (!strcmp(array[i].array_name, str))
			return i;
}