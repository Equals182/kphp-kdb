/*
    This file is part of VK/KittenPHP-DB-Engine.

    VK/KittenPHP-DB-Engine is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    VK/KittenPHP-DB-Engine is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with VK/KittenPHP-DB-Engine.  If not, see <http://www.gnu.org/licenses/>.

    This program is released under the GPL with the additional exemption
    that compiling, linking, and/or using OpenSSL is allowed.
    You are free to remove this exemption from derived works.

    Copyright 2016 Equals182
*/

#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include <string>

#include "kphp_core.h"

OrFalse<string> f$cbN1QL (const array <var, var> &credentials, const string &n1ql);

OrFalse< array<var, var> > f$cbADD(const array <var, var> &credentials, const array <string, string> &data, const bool &wait);
bool f$cbDestroy(const array <var, var> &credentials);