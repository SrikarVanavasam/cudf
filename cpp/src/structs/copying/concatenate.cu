/*
 * Copyright (c) 2020-2023, NVIDIA CORPORATION.
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

#include <cudf/column/column.hpp>
#include <cudf/column/column_device_view.cuh>
#include <cudf/column/column_factories.hpp>
#include <cudf/concatenate.hpp>
#include <cudf/copying.hpp>
#include <cudf/detail/concatenate.cuh>
#include <cudf/detail/get_value.cuh>
#include <cudf/detail/structs/utilities.hpp>
#include <cudf/structs/structs_column_view.hpp>

#include <rmm/cuda_stream_view.hpp>

#include <algorithm>
#include <memory>
#include <numeric>

namespace cudf {
namespace structs {
namespace detail {

/**
 * @copydoc cudf::structs::detail::concatenate
 */
std::unique_ptr<column> concatenate(host_span<column_view const> columns,
                                    rmm::cuda_stream_view stream,
                                    rmm::mr::device_memory_resource* mr)
{
  // get ordered children
  auto ordered_children = extract_ordered_struct_children(columns, stream);

  // concatenate them
  std::vector<std::unique_ptr<column>> children;
  children.reserve(columns[0].num_children());
  std::transform(ordered_children.begin(),
                 ordered_children.end(),
                 std::back_inserter(children),
                 [mr, stream](host_span<column_view const> cols) {
                   return cudf::detail::concatenate(cols, stream, mr);
                 });

  // get total length from concatenated children; if no child exists, we would compute it
  auto const acc_size_fn = [](size_type s, column_view const& c) { return s + c.size(); };
  auto const total_length =
    !children.empty() ? children[0]->size()
                      : std::accumulate(columns.begin(), columns.end(), size_type{0}, acc_size_fn);

  // if any of the input columns have nulls, construct the output mask
  bool const has_nulls =
    std::any_of(columns.begin(), columns.end(), [](auto const& col) { return col.has_nulls(); });
  rmm::device_buffer null_mask =
    create_null_mask(total_length, has_nulls ? mask_state::UNINITIALIZED : mask_state::UNALLOCATED);
  cudf::size_type null_count{0};
  if (has_nulls) {
    cudf::detail::concatenate_masks(columns, static_cast<bitmask_type*>(null_mask.data()), stream);
    null_count =
      std::transform_reduce(columns.begin(), columns.end(), 0, std::plus{}, [](auto const& col) {
        return col.null_count();
      });
  }

  // assemble into outgoing list column
  return make_structs_column(
    total_length, std::move(children), null_count, std::move(null_mask), stream, mr);
}

}  // namespace detail
}  // namespace structs
}  // namespace cudf
