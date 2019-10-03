//==================================================================================
// Module       : gpdf1d.cpp
// Date         : 1/1/19 (DLR)
// Description  : GeoFLOW CGD analysis/postprocessing tool to compute
//                1d PDFs of stored binary output.
// Copyright    : Copyright 2019. Colorado State University. All rights reserved.
// Derived From : none.
//==================================================================================

#include "gexec.h"
#include "gtypes.h"
#include <cstdio>
#include <unistd.h>
#include <iostream>
#include "gtvector.hpp"
#include "gtmatrix.hpp"
#include "gtmatrix.hpp"
#include "gtstat.hpp"
#include "gio.h"
#include "tbox/property_tree.hpp"
#include "tbox/mpixx.hpp"
#include "tbox/global_manager.hpp"
#include "tbox/input_manager.hpp"

using namespace geoflow::tbox;
using namespace std;

struct MyTraits {
  GBOOL    dolog    ;
  GBOOL    bfixeddr ;
  GINT     wfile    ;
  GINT     wtask    ;
  GINT     wtime    ;
  GSIZET   nbins    ;
  GFTYPE   fmin     ;
  GFTYPE   fmax     ;
  GString  idir     ;
  GString  odir     ;
  GString  opref    ;
};

GBOOL init(int &argc, char **argv, PropertyTree &ptree, MyTraits &traits, GTVector<GString> &preflist);


GC_COMM      comm_ ;      // communicator


int main(int argc, char **argv)
{
    GBOOL                bret;
    GINT                 errcode=0, myrank; 
    GString              conig, finput, foutput;
    MyTraits             traits;
    PropertyTree         ptree;
    GTVector<GString>    preflist;

    mpixx::environment env(argc,argv); // init GeoFLOW comm
    mpixx::communicator world;
    GlobalManager::initialize(argc,argv);
    GlobalManager::startup();

    comm_ = world; // need this for solver(s) & grid
    myrank = GComm::WorldRank(comm_);

    bret = init(argc, argv, ptree, traits, preflist);

    assert(bret && "Init failed or no files provided");


    GSIZET                      ipos;
    GIOTraits                   giotraits;
    GTStat<GFTYPE>              gstat(traits.nbins, comm_);
    GTVector<GFTYPE>            u; 
    std::stringstream           sformat;
    GString                     spref;
    char                        stask[16];

    // Currently we only know about GIO, so we assume
    // this is what the data is written in:
    giotraits.wfile  = traits.wfile;
    giotraits.wtask  = traits.wtask;
    giotraits.wtime  = traits.wtime;
    giotraits.dir    = traits.odir;

    // Process each file specified:
    sformat << ".%0" << giotraits.wtask << "d.out";
    sprintf(stask, sformat.str().c_str(), myrank);
    for ( auto i=0; i<preflist.size(); i++ ) {
      // Make sure input file names don't include directory:
#if 0
      ipos    = preflist[i].find("/");
      if ( ipos != std::string::npos ) {
        cout << "    discarding: " << preflist[i] << endl;
        continue;
      }
#endif
      finput  = traits.idir + "/" + preflist[i] + stask;
      foutput = traits.odir + "/" + traits.opref + "_" + preflist[i] + ".txt";

      cout << "main: prefix" << preflist[i] << ": processing " << finput << endl;


      // read in data
      gio_read(giotraits, finput, u);
      gstat.dopdf1d(u, traits.bfixeddr, traits.fmin, traits.fmax, traits.dolog, foutput); 
      cout << "main: pdf written to: " << foutput << "." << endl;
    }


    GlobalManager::shutdown();
    GlobalManager::finalize();


    exit(0);


} // end, main



//**********************************************************************************
//**********************************************************************************
GBOOL init(int &argc, char **argv, PropertyTree &ptree, MyTraits &traits, GTVector<GString> &preflist)
{
    GINT iopt;

#if 0
    // Read main prop tree; may ovewrite with
    // certain command line args:
    EH_MESSAGE("call load prop tree...");
    InputManager::loadInputFile("gpdf1d.jsn");
    ptree     = InputManager::getInputPropertyTree();   
    EH_MESSAGE("prop tree loaded.");

    // Set traits from prop tree, then over write if necessary:
    traits.dolog    = ptree.getValue<GBOOL>  ("dolog"   ,FALSE);
    traits.bfixeddr = ptree.getValue<GBOOL>  ("bfixeddr",FALSE);
    traits.wfile    = ptree.getValue<GINT>   ("wfile"   ,2048);
    traits.wtask    = ptree.getValue<GINT>   ("wtask"   ,5);
    traits.wtime    = ptree.getValue<GINT>   ("wtime"   ,6);
    traits.nbins    = ptree.getValue<GSIZET> ("nbins"   ,500);
    traits.fmin     = ptree.getValue<GFTYPE> ("fmin"    ,0);
    traits.fmax     = ptree.getValue<GFTYPE> ("fmax"    ,100.0);
    traits.idir     = ptree.getValue<GString>("idir"    ,".");
    traits.odir     = ptree.getValue<GString>("odir"    ,".");
    traits.opref    = ptree.getValue<GString>("opref"   ,"pdf1d");
#else
    // Set traits from prop tree, then over write if necessary:
    traits.dolog    = FALSE;
    traits.bfixeddr = FALSE;
    traits.wfile    = 2048;
    traits.wtask    = 5;
    traits.wtime    = 6;
    traits.nbins    = 50;
    traits.fmin     = 0;
    traits.fmax     = 100.0;
    traits.idir     = ".";
    traits.odir     = ".";
    traits.opref    = "pdf1d";
#endif
    
    // Parse command line. ':' after char
    // option indicates that it takes an argument:
    while ((iopt = getopt(argc, argv, "b:d:gl:o:p:u:h")) != -1) {
      // NOTE: -i reserved for Input Manager
      switch (iopt) {
      case 'b': // handled by InputManager
          traits.nbins = atoi(optarg);
          break;
      case 'd': // input dir
          traits.idir = optarg;
          break;
      case 'g': // dolog?
          traits.dolog = TRUE;
          break;
      case 'l': // lower dynamic range
          traits.fmin     = atoi(optarg);
          traits.bfixeddr = TRUE;
          break;
      case 'o': // output dir
          traits.odir = optarg;
          break;
      case 'p': // set output file prefix
          traits.opref = optarg;
          break;
      case 'u': // upper dynamic range
          traits.fmax     = atoi(optarg);
          traits.bfixeddr = TRUE;
          break;
      case ':': // missing option argument
          std::cout << argv[0] << ": option " << optopt << " requires an argument" << std::endl;
          exit(1);
          break;
      case '?':
      case 'h': // help
          std::cout << "usage: " << std::endl <<
          argv[0] << " [-h] [-b #bins] [-d input_dir] [-g dolog?] [-i config_file_name] [-o output_dir] [-p output_file_prefix] [-l lower_dyn_range] [-u upper_dyn_range] file1pref file2pref ..." << std::endl;
          exit(1);
          break;
      default: // invalid option
          std::cout << argv[0] << ": option " << optopt << " invalid" << std::endl;
          exit(1);
          break;
      }
    }

    // Get file prefixes, if any:
    for ( ; optind < argc; optind++ ) {
      preflist.push_back(argv[optind]);
    }

    return preflist.size() > 0;

} // end, doparse method

