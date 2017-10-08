cmake_minimum_required(VERSION 3.0)
project(PdTests)

enable_testing()
find_package(Qt5 5.3 REQUIRED COMPONENTS Core Test)

function(addPdTest TESTNAME TESTSRCS)
    add_executable(Pd_${TESTNAME} ${TESTSRCS})
    setup_score_common_test_features(Pd_${TESTNAME})
    target_link_libraries(Pd_${TESTNAME} PRIVATE Qt5::Core Qt5::Test score_lib_base score_addon_pd sndfile)
    add_test(Pd_${TESTNAME}_target Pd_${TESTNAME})
endFunction()


# Commands
addPdTest(DataflowTest
             "${CMAKE_CURRENT_SOURCE_DIR}/PdDataflowTest.cpp")
