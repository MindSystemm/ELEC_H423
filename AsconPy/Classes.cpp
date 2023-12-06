#include "Classes.h"

uint8_t* py_arr_to_uint8_t(py::array_t<uint8_t> arr) {
    py::buffer_info info = arr.request();
    uint8_t* ptr = static_cast<uint8_t *>(info.ptr);
    return ptr;
}

py::array_t<uint8_t> uint8_t_arr_to_py(uint8_t* arr, size_t l) {
    py::array_t<uint8_t> result = py::array_t<uint8_t>({l});
    py::buffer_info res_buf = result.request();
    uint8_t* ptr_res = static_cast<uint8_t *>(res_buf.ptr);

    memcpy(ptr_res, arr, l);

    return result;
}