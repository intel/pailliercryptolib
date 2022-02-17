# Unit tests

# main unittest
add_executable(unit-test test.cpp)
target_include_directories(unit-test PUBLIC ${IPCL_INC_DIR})
target_link_libraries(unit-test PRIVATE ipcl libgtest Threads::Threads)

# omp unittest
if(IPCL_TEST_OMP)
  find_package(OpenMP REQUIRED)

  add_executable(unit-test_omp test_omp.cpp)
  target_include_directories(unit-test_omp PUBLIC ${IPCL_INC_DIR})
  target_link_libraries(unit-test_omp PRIVATE ipcl OpenMP::OpenMP_CXX libgtest Threads::Threads)
endif()
