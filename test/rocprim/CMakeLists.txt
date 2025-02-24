# MIT License
#
# Copyright (c) 2017-2022 Advanced Micro Devices, Inc. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

set(AMDGPU_TEST_TARGETS "" CACHE STRING "List of specific device types to test for") # Leave empty for default system device

function(add_rocprim_test TEST_NAME TEST_SOURCES)
  list(GET TEST_SOURCES 0 TEST_MAIN_SOURCE)
  get_filename_component(TEST_TARGET ${TEST_MAIN_SOURCE} NAME_WE)

  add_executable(${TEST_TARGET} ${TEST_SOURCES})
  rocm_install(TARGETS ${TEST_TARGET} COMPONENT tests)

  target_include_directories(${TEST_TARGET} SYSTEM BEFORE
    PUBLIC
      $<BUILD_INTERFACE:${COMMON_TEST_HEADER_DIRECTORY}>
  )

  if(TARGET GTest::GTest)
    target_link_libraries(${TEST_TARGET}
      PRIVATE
        GTest::GTest
        GTest::Main
    )
  else()
    target_link_libraries(${TEST_TARGET}
      PRIVATE
        GTest::gtest
        GTest::gtest_main
    )
  endif()
  if(NOT USE_HIP_CPU)
    target_link_libraries(${TEST_TARGET}
      PRIVATE
        rocprim_hip
    )
  else()
    target_link_libraries(${TEST_TARGET}
      PRIVATE
        rocprim
        Threads::Threads
        hip_cpu_rt::hip_cpu_rt
    )
    if(STL_DEPENDS_ON_TBB)
      target_link_libraries(${TEST_TARGET}
        PRIVATE
          TBB::tbb
      )
    endif()
  endif()

  target_compile_options(${TEST_TARGET}
    PRIVATE
      $<$<CXX_COMPILER_ID:MSVC>:
        /bigobj # number of sections exceeded object file format limit: compile with /bigobj
      >
  )

  set_target_properties(${TEST_TARGET}
    PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/rocprim"
  )
  if(AMDGPU_TEST_TARGETS)
    foreach(AMDGPU_TARGET IN LISTS AMDGPU_TEST_TARGETS)
      add_relative_test("${AMDGPU_TARGET}-${TEST_NAME}" ${TEST_TARGET})
      set_tests_properties("${AMDGPU_TARGET}-${TEST_NAME}"
          PROPERTIES
              RESOURCE_GROUPS "1,${AMDGPU_TARGET}:1"
              LABELS "hip;${AMDGPU_TARGET}"
      )
    endforeach()
  else()
      add_relative_test(${TEST_NAME} ${TEST_TARGET})
      set_tests_properties(${TEST_NAME}
          PROPERTIES
              LABELS "hip"
      )
  endif()

  if (WIN32 AND NOT DEFINED DLLS_COPIED_2)
    set(DLLS_COPIED_2 "YES")
    set(DLLS_COPIED_2 ${DLLS_COPIED_2} PARENT_SCOPE)
    # for now adding in all .dll as dependency chain is not cmake based on win32
    file( GLOB third_party_dlls
    LIST_DIRECTORIES ON
    CONFIGURE_DEPENDS
    ${HIP_DIR}/bin/*.dll
    ${CMAKE_SOURCE_DIR}/rtest.*
    )
    foreach( file_i ${third_party_dlls})
      add_custom_command( TARGET ${TEST_TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${file_i} ${PROJECT_BINARY_DIR}/test/rocprim )
    endforeach( file_i )
  endif()
endfunction()

# ****************************************************************************
# Tests
# ****************************************************************************
set(BLOCK_ADJACENT_DIFFERENCE_SOURCES_LIST test_block_adjacent_difference.floating.cpp
                                           test_block_adjacent_difference.integral.cpp)

set(DEVICE_RADIX_SORT_SOURCES_LIST test_device_radix_sort.sortkeys.cpp
                                           test_device_radix_sort.sortkeysdouble.cpp
                                           test_device_radix_sort.sortpairs.cpp
                                           test_device_radix_sort.sortpairsdouble.cpp
                                           test_device_radix_sort.sortkeysover4g.cpp)

set(DEVICE_SEGMENTED_RADIX_SORT_SOURCES_LIST test_device_segmented_radix_sort.sortkeys.cpp
                                             test_device_segmented_radix_sort.sortpairs.cpp)

# HIP basic test, which also checks if there are no linkage problems when there are multiple sources
add_rocprim_test("rocprim.basic_test" "test_basic.cpp;detail/get_rocprim_version.cpp")

add_rocprim_test("rocprim.arg_index_iterator" test_arg_index_iterator.cpp)
add_rocprim_test("rocprim.block_adjacent_difference" "${BLOCK_ADJACENT_DIFFERENCE_SOURCES_LIST}")
add_rocprim_test("rocprim.block_discontinuity" test_block_discontinuity.cpp)
add_rocprim_test("rocprim.block_exchange" test_block_exchange.cpp)
add_rocprim_test("rocprim.block_histogram" test_block_histogram.cpp)
add_rocprim_test("rocprim.block_load_store" test_block_load_store.cpp)
add_rocprim_test("rocprim.block_radix_sort" test_block_radix_sort.cpp)
add_rocprim_test("rocprim.block_reduce" test_block_reduce.cpp)
add_rocprim_test("rocprim.block_scan" test_block_scan.cpp)
add_rocprim_test("rocprim.block_shuffle" test_block_shuffle.cpp)
add_rocprim_test("rocprim.block_sort" test_block_sort.cpp)
add_rocprim_test("rocprim.constant_iterator" test_constant_iterator.cpp)
add_rocprim_test("rocprim.counting_iterator" test_counting_iterator.cpp)
add_rocprim_test("rocprim.device_binary_search" test_device_binary_search.cpp)
add_rocprim_test("rocprim.device_adjacent_difference" test_device_adjacent_difference.cpp)
add_rocprim_test("rocprim.device_histogram" test_device_histogram.cpp)
add_rocprim_test("rocprim.device_merge" test_device_merge.cpp)
add_rocprim_test("rocprim.device_merge_sort" test_device_merge_sort.cpp)
add_rocprim_test("rocprim.device_partition" test_device_partition.cpp)
add_rocprim_test("rocprim.device_radix_sort" "${DEVICE_RADIX_SORT_SOURCES_LIST}")
add_rocprim_test("rocprim.device_reduce_by_key" test_device_reduce_by_key.cpp)
add_rocprim_test("rocprim.device_reduce" test_device_reduce.cpp)
add_rocprim_test("rocprim.device_run_length_encode" test_device_run_length_encode.cpp)
add_rocprim_test("rocprim.device_scan" test_device_scan.cpp)
add_rocprim_test("rocprim.device_segmented_radix_sort" "${DEVICE_SEGMENTED_RADIX_SORT_SOURCES_LIST}")
add_rocprim_test("rocprim.device_segmented_reduce" test_device_segmented_reduce.cpp)
add_rocprim_test("rocprim.device_segmented_scan" test_device_segmented_scan.cpp)
add_rocprim_test("rocprim.device_select" test_device_select.cpp)
add_rocprim_test("rocprim.device_transform" test_device_transform.cpp)
add_rocprim_test("rocprim.discard_iterator" test_discard_iterator.cpp)
if(NOT USE_HIP_CPU)
  add_rocprim_test("rocprim.texture_cache_iterator" test_texture_cache_iterator.cpp)
endif()
add_rocprim_test("rocprim.thread" test_thread.cpp)
add_rocprim_test("rocprim.thread_algos" test_thread_algos.cpp)
add_rocprim_test("rocprim.transform_iterator" test_transform_iterator.cpp)
add_rocprim_test("rocprim.intrinsics" test_intrinsics.cpp)
add_rocprim_test("rocprim.warp_exchange" test_warp_exchange.cpp)
add_rocprim_test("rocprim.warp_load" test_warp_load.cpp)
add_rocprim_test("rocprim.warp_reduce" test_warp_reduce.cpp)
add_rocprim_test("rocprim.warp_scan" test_warp_scan.cpp)
add_rocprim_test("rocprim.warp_sort" test_warp_sort.cpp)
add_rocprim_test("rocprim.warp_store" test_warp_store.cpp)
add_rocprim_test("rocprim.zip_iterator" test_zip_iterator.cpp)
