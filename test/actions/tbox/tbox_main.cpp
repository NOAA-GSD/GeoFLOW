/*
 * tbox.cpp
 *
 *  Created on: Nov 13, 2018
 *      Author: bflynt
 */

#include "tbox/pio.hpp"
#include "tbox/mpixx.hpp"
#include "tbox/global_manager.hpp"
#include "tbox/input_manager.hpp"

using namespace geoflow::tbox;

int main(int argc, char* argv[]) {

	// Start Up MPI
	mpixx::environment env(argc,argv);
	mpixx::communicator world;

	// Initialize global (once per run)
	GlobalManager::initialize(argc,argv);

	// Call startup call backs
	GlobalManager::startup();

	pio::perr << "Testing error stream\n";

	// Call shutdown call backs
	GlobalManager::shutdown();

	// Finalize global (once per run)
	GlobalManager::finalize();

	return 0;
}
