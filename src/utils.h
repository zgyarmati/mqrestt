/*
 *  This file is part of mqrestt.
 *
 *  Mqrestt is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Mqrestt is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with mqrestt.  If not, see <https://www.gnu.org/licenses/>.
 *
 *   Copyright  Zoltan Gyarmati <zgyarmati@zgyarmati.de> 2020
 */

#ifndef UTILS_H
#define UTILS_H
#include <stdlib.h>
#include <sys/types.h>
#include <stdbool.h>
#include <errno.h>
#include <limits.h>

bool parseInt(const char *str, int *val);

void* safe_malloc(size_t n, unsigned long line);
#define SAFEMALLOC(n) safe_malloc(n, __LINE__)
void* safe_realloc(void *ptr, size_t n, unsigned long line);
#define SAFEREALLOC(ptr, n) safe_realloc(ptr, n, __LINE__)


#endif


