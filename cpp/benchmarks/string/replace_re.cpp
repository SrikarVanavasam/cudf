/*
 * Copyright (c) 2021-2023, NVIDIA CORPORATION.
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

#include "string_bench_args.hpp"

#include <benchmarks/common/generate_input.hpp>
#include <benchmarks/fixture/benchmark_fixture.hpp>
#include <benchmarks/synchronization/synchronization.hpp>

#include <cudf_test/column_wrapper.hpp>

#include <cudf/strings/regex/regex_program.hpp>
#include <cudf/strings/replace_re.hpp>
#include <cudf/strings/strings_column_view.hpp>
#include <cudf/utilities/default_stream.hpp>

class StringReplace : public cudf::benchmark {};

enum replace_type { replace_re, replace_re_multi, replace_backref };

static void BM_replace(benchmark::State& state, replace_type rt)
{
  cudf::size_type const n_rows{static_cast<cudf::size_type>(state.range(0))};
  cudf::size_type const max_str_length{static_cast<cudf::size_type>(state.range(1))};
  data_profile const profile = data_profile_builder().distribution(
    cudf::type_id::STRING, distribution_id::NORMAL, 0, max_str_length);
  auto const column = create_random_column(cudf::type_id::STRING, row_count{n_rows}, profile);
  cudf::strings_column_view input(column->view());
  cudf::test::strings_column_wrapper repls({"#", ""});
  auto prog         = cudf::strings::regex_program::create("\\d+");
  auto prog_backref = cudf::strings::regex_program::create("(\\d+)");

  for (auto _ : state) {
    cuda_event_timer raii(state, true, cudf::get_default_stream());
    switch (rt) {
      case replace_type::replace_re:  // contains_re and matches_re use the same main logic
        cudf::strings::replace_re(input, *prog);
        break;
      case replace_type::replace_re_multi:  // counts occurrences of pattern
        cudf::strings::replace_re(input, {"\\d+", "\\s+"}, cudf::strings_column_view(repls));
        break;
      case replace_type::replace_backref:  // returns occurrences of matches
        cudf::strings::replace_with_backrefs(input, *prog_backref, "#\\1X");
        break;
    }
  }

  state.SetBytesProcessed(state.iterations() * input.chars_size());
}

static void generate_bench_args(benchmark::internal::Benchmark* b)
{
  int const min_rows   = 1 << 12;
  int const max_rows   = 1 << 24;
  int const row_mult   = 8;
  int const min_rowlen = 1 << 5;
  int const max_rowlen = 1 << 13;
  int const len_mult   = 4;
  generate_string_bench_args(b, min_rows, max_rows, row_mult, min_rowlen, max_rowlen, len_mult);
}

#define STRINGS_BENCHMARK_DEFINE(name)                \
  BENCHMARK_DEFINE_F(StringReplace, name)             \
  (::benchmark::State & st) { BM_replace(st, name); } \
  BENCHMARK_REGISTER_F(StringReplace, name)           \
    ->Apply(generate_bench_args)                      \
    ->UseManualTime()                                 \
    ->Unit(benchmark::kMillisecond);

STRINGS_BENCHMARK_DEFINE(replace_re)
STRINGS_BENCHMARK_DEFINE(replace_re_multi)
STRINGS_BENCHMARK_DEFINE(replace_backref)
