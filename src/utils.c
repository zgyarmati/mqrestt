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
 *   Copyright  Zoltan Gyarmati <zgyarmati@zgyarmati.de> 2021
 */

#include "utils.h"
#include "stdio.h"

bool parseInt(const char *str, int *val)
{
    char *temp;
    bool rc = true;
    errno = 0;
    long res = strtol(str, &temp, 0);

    if (temp == str || *temp != '\0' ||
        ((*val == LONG_MIN || *val == LONG_MAX) && errno == ERANGE))
        rc = false;
    *val = (int)res;
    return rc;
}

void *safe_malloc(size_t n, unsigned long line)
{
    void *p = malloc(n);
    if (!p) {
        fprintf(stderr, "[%s:%lu]Out of memory(%lu bytes)\n", __FILE__, line,
                (unsigned long)n);
        exit(EXIT_FAILURE);
    }
    return p;
}

void *safe_realloc(void *ptr, size_t n, unsigned long line)
{
    void *p = realloc(ptr, n);
    if (!p) {
        fprintf(stderr, "[%s:%lu]Out of memory(%lu bytes)\n", __FILE__, line,
                (unsigned long)n);
        exit(EXIT_FAILURE);
    }
    return p;
}
