#include <iostream>

#include <Pythia8/Pythia.h>
#include <Pythia8Plugins/PowhegHooks.h>

#include <TFile.h>
#include <TTree.h>
#include <ROOT/RVec.hxx>

#define MAXPARTICLES 4096

int main(int argc, char* argv[])
{
  //
  // Check arguments
  if(argc!=3)
    {
      std::cerr << "usage: " << argv[0] << " pythia.cmnd data.lhe" << std::endl;
      return 1;
    }
  std::string cmnd=argv[1];
  std::string lhe =argv[2];

  // Initialize ROOT output
  TFile *fh=TFile::Open("output.root","RECREATE");

  TTree *t=new TTree("outTree","outTree");

  uint32_t br_nparticles;
  t->Branch("nparticles"    , &br_nparticles,"nparticles/i");
  
  float   br_particles_pt    [MAXPARTICLES];
  t->Branch("particles_pt"    , &br_particles_pt    ,"particles_pt[nparticles]/F"    );

  float   br_particles_eta   [MAXPARTICLES];
  t->Branch("particles_eta"   , &br_particles_eta   ,"particles_eta[nparticles]/F"   );

  float   br_particles_phi   [MAXPARTICLES];
  t->Branch("particles_phi"   , &br_particles_phi   ,"particles_phi[nparticles]/F"   );

  float   br_particles_m     [MAXPARTICLES];
  t->Branch("particles_m"     , &br_particles_m     ,"particles_m[nparticles]/F"     );

  int32_t br_particles_pdg   [MAXPARTICLES];
  t->Branch("particles_pdg"   , &br_particles_pdg   ,"particles_pdg[nparticles]/I"   );

  int32_t br_particles_status[MAXPARTICLES];
  t->Branch("particles_status", &br_particles_status,"particles_status[nparticles]/I");
  
  //
  // Initialize Pythia
  Pythia8::Pythia pythia;

  // Load configuration file
  pythia.readFile(cmnd);
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
  pythia.init();

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
      br_nparticles=pythia.event.size();
      for(uint32_t i=0;i<br_nparticles;++i)
	{
	  br_particles_pt    [i]=pythia.event[i].pT    ();
	  br_particles_eta   [i]=pythia.event[i].eta   ();
	  br_particles_phi   [i]=pythia.event[i].phi   ();
	  br_particles_m     [i]=pythia.event[i].m     ();
	  br_particles_pdg   [i]=pythia.event[i].id    ();
	  br_particles_status[i]=pythia.event[i].status();
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
