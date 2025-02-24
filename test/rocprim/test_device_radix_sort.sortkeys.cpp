// MIT License
//
// Copyright (c) 2017-2022 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "test_device_radix_sort.hpp"

TYPED_TEST(RocprimDeviceRadixSort, SortKeys)
{
    int device_id = test_common_utils::obtain_device_from_ctest();
    SCOPED_TRACE(testing::Message() << "with device_id= " << device_id);
    HIP_CHECK(hipSetDevice(device_id));

    using key_type = typename TestFixture::params::key_type;
    constexpr bool descending = TestFixture::params::descending;
    constexpr unsigned int start_bit = TestFixture::params::start_bit;
    constexpr unsigned int end_bit = TestFixture::params::end_bit;
    constexpr bool check_large_sizes = TestFixture::params::check_large_sizes;

    hipStream_t stream = 0;

    const bool debug_synchronous = false;

    bool in_place = false;

    for (size_t seed_index = 0; seed_index < random_seeds_count + seed_size; seed_index++)
    {
        unsigned int seed_value = seed_index < random_seeds_count  ? rand() : seeds[seed_index - random_seeds_count];
        SCOPED_TRACE(testing::Message() << "with seed= " << seed_value);

        for(unsigned int size : get_sizes(seed_value))
        {
            if(size > (1 << 20) && !check_large_sizes) continue;
            if (size == 0 && test_common_utils::use_hmm())
            {
                // hipMallocManaged() currently doesnt support zero byte allocation
                continue;
            }

            SCOPED_TRACE(testing::Message() << "with size = " << size);

            in_place = !in_place;

            // Generate data
            std::vector<key_type> keys_input;
            if(rocprim::is_floating_point<key_type>::value)
            {
                keys_input = test_utils::get_random_data<key_type>(size, (key_type)-1000, (key_type)+1000, seed_value);
                test_utils::add_special_values(keys_input, seed_value);
            }
            else
            {
                keys_input = test_utils::get_random_data<key_type>(
                    size,
                    std::numeric_limits<key_type>::min(),
                    std::numeric_limits<key_type>::max(),
                    seed_index
                );
            }

            key_type * d_keys_input;
            key_type * d_keys_output;
            HIP_CHECK(test_common_utils::hipMallocHelper(&d_keys_input, size * sizeof(key_type)));
            if(in_place)
            {
                d_keys_output = d_keys_input;
            }
            else
            {
                HIP_CHECK(test_common_utils::hipMallocHelper(&d_keys_output, size * sizeof(key_type)));
            }
            HIP_CHECK(
                hipMemcpy(
                    d_keys_input, keys_input.data(),
                    size * sizeof(key_type),
                    hipMemcpyHostToDevice
                )
            );

            // Calculate expected results on host
            std::vector<key_type> expected(keys_input);
            std::stable_sort(expected.begin(), expected.end(), test_utils::key_comparator<key_type, descending, start_bit, end_bit>());

            // Use custom config
            using config = rocprim::radix_sort_config<8, 5, rocprim::kernel_config<256, 3>, rocprim::kernel_config<256, 8>>;

            size_t temporary_storage_bytes;
            HIP_CHECK(
                rocprim::radix_sort_keys<config>(
                    nullptr, temporary_storage_bytes,
                    d_keys_input, d_keys_output, size,
                    start_bit, end_bit
                )
            );

            ASSERT_GT(temporary_storage_bytes, 0);

            void * d_temporary_storage;
            HIP_CHECK(test_common_utils::hipMallocHelper(&d_temporary_storage, temporary_storage_bytes));

            if(descending)
            {
                HIP_CHECK(
                    rocprim::radix_sort_keys_desc<config>(
                        d_temporary_storage, temporary_storage_bytes,
                        d_keys_input, d_keys_output, size,
                        start_bit, end_bit,
                        stream, debug_synchronous
                    )
                );
            }
            else
            {
                HIP_CHECK(
                    rocprim::radix_sort_keys<config>(
                        d_temporary_storage, temporary_storage_bytes,
                        d_keys_input, d_keys_output, size,
                        start_bit, end_bit,
                        stream, debug_synchronous
                    )
                );
            }

            std::vector<key_type> keys_output(size);
            HIP_CHECK(
                hipMemcpy(
                    keys_output.data(), d_keys_output,
                    size * sizeof(key_type),
                    hipMemcpyDeviceToHost
                )
            );

            HIP_CHECK(hipFree(d_temporary_storage));
            HIP_CHECK(hipFree(d_keys_input));
            if(!in_place)
            {
                HIP_CHECK(hipFree(d_keys_output));
            }

            ASSERT_NO_FATAL_FAILURE(test_utils::assert_bit_eq(keys_output, expected));
        }
    }
}
