/*
 * Copyright (c) 2023, NVIDIA CORPORATION.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <cudf/contiguous_split.hpp>
#include <cudf/types.hpp>
#include <cudf/utilities/default_stream.hpp>

#include <rmm/cuda_stream_view.hpp>

namespace cudf {
namespace detail {

/**
 * @copydoc cudf::contiguous_split
 *
 * @param stream CUDA stream used for device memory operations and kernel launches.
 **/
std::vector<packed_table> contiguous_split(cudf::table_view const& input,
                                           std::vector<size_type> const& splits,
                                           rmm::cuda_stream_view stream,
                                           rmm::mr::device_memory_resource* mr);

/**
 * @copydoc cudf::pack
 *
 * @param stream Optional CUDA stream on which to execute kernels
 **/
packed_columns pack(cudf::table_view const& input,
                    rmm::cuda_stream_view stream,
                    rmm::mr::device_memory_resource* mr);

}  // namespace detail
}  // namespace cudf
