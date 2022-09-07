/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*  CFL
 *  ===
 *  Copyright (C) 2022 The CFL Authors
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef CFL_H
#define CFL_H

#define CFL_FALSE   0
#define CFL_TRUE    !CFL_FALSE

/* headers that are needed in general */
#include <cfl/cfl_info.h>
#include <cfl/cfl_version.h>
#include <cfl/cfl_log.h>
#include <cfl/cfl_sds.h>
#include <cfl/cfl_list.h>
#include <cfl/cfl_hash.h>

int clf_init();

#endif
