#include <iostream>

#include <ROOT/RDataFrame.hxx>

#include "RPythiaSource.h"

int main(int argc, char* argv[])
{
  //
  // Check arguments
  if(argc<2)
    {
      std::cerr << "usage: " << argv[0] << " pythia.cmnd" << std::endl;
      return 1;
    }

  //
  // Prepare input
  //ROOT::EnableImplicitMT(1);
  ROOT::RDataFrame df(std::make_unique<RPythiaSource>(argv[1],100));

  df.Snapshot("outTree","test.root",{"nparticles"});
  //df.Display({"particles_pt"})->Print();
  
  return 0;
}
