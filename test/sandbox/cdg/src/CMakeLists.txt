#
# === Unit Tests
#

# 
# Build list of all tests to create
#
set(test_cdg_files
		# gtest_hack.cpp
		# gtest_mass.cpp
		# gtest_ylm.cpp
		# gtest_gll.cpp
		# gtest_gmm.cpp
		# gtest_helm.cpp
		# gtest_morton.cpp
		# gtest_mult.cpp
		# gtest_pingpong.cpp
		# gtest_rk.cpp
		# gtest_vec_access.cpp
		# gtest_vec.cpp
)

#
# Build list of compiler options for tests
#
set(compiler_options 
		"$<$<CXX_COMPILER_ID:AppleClang>:-Wno-comment>"
	)
		       
# Set include headers unique to testing
set(include_headers "")

# Set libraries the test need
# - Gets library include paths from here
set(linked_libraries 
		cdg 
		"$<$<BOOL:${OpenMP_FOUND}>:OpenMP::OpenMP_CXX>"
		"$<$<BOOL:${MPI_CXX_FOUND}>:MPI::MPI_CXX>"
		"$<$<BOOL:${Boost_SERIALIZATION_FOUND}>:Boost::serialization>"
		"$<$<BOOL:${Boost_MPI_FOUND}>:Boost::mpi>"
		"$<$<BOOL:${PAPI_FOUND}>:PAPI::PAPI>"
		"$<$<BOOL:${GPTL_FOUND}>:GPTL::GPTL>"
	)
	      
#
# Loop over each test building
# - Build test name
#	             
FOREACH(test_src ${test_cdg_files})
	string(REPLACE ".cpp" "" test_name ${test_src})
	
	add_executable(${test_name} ${test_src})
	target_compile_options(${test_name} PRIVATE ${compiler_options})
	target_include_directories(${test_name} PRIVATE ${include_headers})
	target_link_libraries(${test_name} ${linked_libraries})
	install(TARGETS ${test_name} DESTINATION  "${MY_INSTALL_TSTDIR}/sandbox/cdg")
ENDFOREACH(test_src)



