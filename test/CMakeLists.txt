# Unit tests

# main unittest
add_executable(unit-test test.cpp)
target_link_libraries(unit-test PRIVATE ipcl libgtest Threads::Threads)
target_include_directories(unit-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${IPCL_INC_DIR})# ${IPPCRYPTO_INC_DIR})

# omp unittest
if(IPCL_TEST_OMP)
  find_package(OpenMP REQUIRED)

  add_executable(unit-test_omp test_omp.cpp)
  target_link_libraries(unit-test_omp PRIVATE ipcl OpenMP::OpenMP_CXX libgtest Threads::Threads)
  target_include_directories(unit-test_omp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${IPCL_INC_DIR})# ${IPPCRYPTO_INC_DIR})
endif()
