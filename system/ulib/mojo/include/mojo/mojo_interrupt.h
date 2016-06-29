// Copyright 2016 The Fuchsia Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <mojo/mojo_types.h>
#include <runtime/compiler.h>

__BEGIN_CDECLS;

mojo_result_t mojo_interrupt_event_create(uint32_t vector, uint32_t flags, mojo_handle_t* handle);

mojo_result_t mojo_interrupt_event_complete(mojo_handle_t handle);

__END_CDECLS;