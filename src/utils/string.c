/*
 * utils/string.h - string utils
 *
 * Copyright (C) 2018 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <string.h>

#include "utils/string.h"

#include <gtk/gtk.h>

int
string_is_ascii (const char * string)
{
  unsigned long i;
  if (!string || strlen (string) == 0)
    return 0;
  for (i = 0; i < strlen (string); i++)
    {
      if (string[i] < 32 ||
          string[i] > 126)
        {
          return 0;
        }
    }
  return 1;
}

/**
 * Returns the matched string if the string array
 * contains the given substring.
 */
char *
string_array_contains_substr (
  char ** str_array,
  int     num_str,
  char *  substr)
{
  for (int i = 0; i < num_str; i++)
    {
      if (g_str_match_string (
        substr,
        str_array[i],
        0))
        return str_array[i];
    }

  return NULL;
}

/**
 * Returns if the two strings are equal.
 */
int
string_is_equal (
  const char * str1,
  const char * str2,
  int          ignore_case)
{
  if (ignore_case)
    {
      char * str1_casefolded =
        g_utf8_casefold (
          str1, -1);
      char * str2_casefolded =
        g_utf8_casefold (
          str2, -1);
      int ret =
        !g_strcmp0 (
          str1_casefolded,
          str2_casefolded);
      g_free (str1_casefolded);
      g_free (str2_casefolded);
      return ret;
    }
  else
    {
      return !g_strcmp0 (str1, str2);
    }
}
