# Copyright (c) 2021, NVIDIA CORPORATION.

from libcpp.memory cimport unique_ptr
from libcpp.utility cimport move
from libcpp cimport bool

from cudf._lib.column cimport Column
from cudf._lib.cpp.column.column cimport column
from cudf._lib.cpp.column.column_view cimport column_view
from cudf._lib.cpp.scalar.scalar cimport string_scalar
from cudf._lib.cpp.strings.json cimport (get_json_object as cpp_get_json_object, get_json_object_options)
from cudf._lib.cpp.types cimport size_type
from cudf._lib.scalar cimport DeviceScalar


def get_json_object(Column col, object py_json_path, bool allow_single_quotes=False, bool strip_quotes_from_single_strings=True, bool missing_fields_as_nulls=False):
    """
    Apply a JSONPath string to all rows in an input column
    of json strings.
    """
    cdef unique_ptr[column] c_result

    cdef column_view col_view = col.view()
    cdef DeviceScalar json_path = py_json_path.device_value

    cdef const string_scalar* scalar_json_path = <const string_scalar*>(
        json_path.get_raw_ptr()
    )

    cdef get_json_object_options options
    options.set_allow_single_quotes(allow_single_quotes)
    options.set_strip_quotes_from_single_strings(strip_quotes_from_single_strings)
    options.set_missing_fields_as_nulls(missing_fields_as_nulls)

    with nogil:
        c_result = move(cpp_get_json_object(
            col_view,
            scalar_json_path[0],
            options,
        ))

    return Column.from_unique_ptr(move(c_result))
