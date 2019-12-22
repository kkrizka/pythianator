#include <iostream>

#include <ROOT/RDataFrame.hxx>

#include "RPythiaSource.h"

int main(int argc, char* argv[])
{
  //
  // Check arguments
  if(argc!=3)
    {
      std::cerr << "usage: " << argv[0] << " pythia.cmnd nEvents" << std::endl;
      return 1;
    }
  std::string cmnd=argv[1];
  uint32_t nEvents=std::stoul(argv[2]);

  //
  // Prepare input
  ROOT::EnableImplicitMT();
  ROOT::RDataFrame df(std::make_unique<RPythiaSource>(cmnd,nEvents));

  df.Snapshot("outTree","test.root",{"nparticles","particles_pt","particles_eta","particles_phi","particles_m","particles_pdg","particles_status"});
  
  return 0;
}
