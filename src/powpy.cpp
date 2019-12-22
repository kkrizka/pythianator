#include <iostream>

#include <Pythia8/Pythia.h>
#include <Pythia8Plugins/PowhegHooks.h>

#include <TFile.h>
#include <TTree.h>
#include <ROOT/RVec.hxx>

#define MAXPARTICLES 8192

int main(int argc, char* argv[])
{
  //
  // Check arguments
  if(argc!=4)
    {
      std::cerr << "usage: " << argv[0] << " pythia.cmnd data.lhe output.root" << std::endl;
      return 1;
    }
  std::string cmnd=argv[1];
  std::string lhe =argv[2];
  std::string outp=argv[3];

  // Initialize ROOT output
  TFile *fh=TFile::Open(outp.c_str(),"RECREATE");

  TTree *t=new TTree("outTree","outTree");

  float br_weight;
  t->Branch("weight"        , &br_weight    ,"weight/F"    );

  uint32_t br_nparticle;
  t->Branch("nparticle"    , &br_nparticle,"nparticle/i");
  
  float   br_particle_pt    [MAXPARTICLES];
  t->Branch("particle_pt"    , &br_particle_pt    ,"particle_pt[nparticle]/F"    );

  float   br_particle_eta   [MAXPARTICLES];
  t->Branch("particle_eta"   , &br_particle_eta   ,"particle_eta[nparticle]/F"   );

  float   br_particle_phi   [MAXPARTICLES];
  t->Branch("particle_phi"   , &br_particle_phi   ,"particle_phi[nparticle]/F"   );

  float   br_particle_m     [MAXPARTICLES];
  t->Branch("particle_m"     , &br_particle_m     ,"particle_m[nparticle]/F"     );

  int32_t br_particle_pdg   [MAXPARTICLES];
  t->Branch("particle_pdg"   , &br_particle_pdg   ,"particle_pdg[nparticle]/I"   );

  int32_t br_particle_status[MAXPARTICLES];
  t->Branch("particle_status", &br_particle_status,"particle_status[nparticle]/I");
  
  //
  // Initialize Pythia
  Pythia8::Pythia pythia;

  // Load configuration file
  pythia.readFile(cmnd);
  pythia.readString("Main:numberOfEvents = 0");
  pythia.readString("Beams:frameType = 4");
  pythia.readString("Beams:LHEF = "+lhe);

  // Read in main settings
  int nEvent      = pythia.settings.mode("Main:numberOfEvents");
  int nError      = pythia.settings.mode("Main:timesAllowErrors");
  // Read in key POWHEG merging settings
  int vetoMode    = pythia.settings.mode("POWHEG:veto");
  int MPIvetoMode = pythia.settings.mode("POWHEG:MPIveto");
  bool loadHooks  = (vetoMode > 0 || MPIvetoMode > 0);

  // Add in user hooks for shower vetoing
  Pythia8::PowhegHooks *powhegHooks = nullptr;
  if(loadHooks)
    {
      // Set ISR and FSR to start at the kinematical limit
      if(vetoMode>0)
	{
	  pythia.readString("SpaceShower:pTmaxMatch = 2");
	  pythia.readString("TimeShower:pTmaxMatch = 2");
	}

      // Set MPI to start at the kinematical limit
      if(MPIvetoMode > 0)
	{
	  pythia.readString("MultipartonInteractions:pTmaxMatch = 2");
	}

      powhegHooks = new Pythia8::PowhegHooks();
      pythia.setUserHooksPtr((Pythia8::UserHooks *) powhegHooks);
    }

  // Initialize and list settings
  if(!pythia.init())
    {
      std::cerr << "Error: initialize" << std::endl;
      return 1;
    }

  // Counters for number of ISR/FSR emissions vetoed
  unsigned long int nISRveto = 0, nFSRveto = 0;

  // Begin event loop; generate until nEvent events are processed
  // of end of LHEF file
  int iEvent = 0, iError = 0;

  while(true)
    {
      // Generate the next event
      if(!pythia.next())
	{ // Handle error!
	  // If failure because reached of of file, then exit event loop
	  if(pythia.info.atEndOfFile()) break;

	  // Otherwise count event failure and continue/exit as necessary
	  std::cerr << "Warning: event " << iError << " failed" << std::endl;
	  if(++iError == nError)
	    {
	      std::cerr << "Error: too many event failures.. existing" << std::endl;
	      break;
	    }

	  continue;
	}

      // Save the event
      br_weight=pythia.info.weight();

      uint32_t nPart=pythia.event.size();
      if(nPart>MAXPARTICLES)
	{
	  std::cerr << "Error: Too many particles " << br_nparticle << std::endl;
	  return 1;
	}

      br_nparticle=0;
      for(uint32_t i=0;i<nPart;++i)
	{
	  if(!pythia.event[i].isResonance() && !pythia.event[i].isFinal())
	    continue;
	  br_particle_pt    [br_nparticle]=pythia.event[i].pT    ();
	  br_particle_eta   [br_nparticle]=pythia.event[i].eta   ();
	  br_particle_phi   [br_nparticle]=pythia.event[i].phi   ();
	  br_particle_m     [br_nparticle]=pythia.event[i].m     ();
	  br_particle_pdg   [br_nparticle]=pythia.event[i].id    ();
	  br_particle_status[br_nparticle]=pythia.event[i].status();
	  ++br_nparticle;
	}
      t->Fill();

      // Update ISR/FSR veto counters
      if(loadHooks)
	{
	  nISRveto += powhegHooks->getNISRveto();
	  nFSRveto += powhegHooks->getNFSRveto();
	}

      // If nEvent is set, check and exit loop if necessary
      ++iEvent;
      if(nEvent!=0 && iEvent==nEvent) break;
    }

  // Statistics, histograms and veto information
  pythia.stat();
  std::cout << "Number of ISR emissions vetoed: " << nISRveto << std::endl;
  std::cout << "Number of FSR emissions vetoed: " << nFSRveto << std::endl;
  
  // Done
  if(powhegHooks) delete powhegHooks;
  fh->Write();
  fh->Close();
  
  return 0;
}
