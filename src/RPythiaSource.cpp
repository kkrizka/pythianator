#include "RPythiaSource.h"

#include <iostream>

RPythiaSource::RPythiaSource(const std::string& cmnd, ULong64_t nEvents)
  : m_cmnd(cmnd), m_nEvents(nEvents)
{ }

const std::vector<std::string>& RPythiaSource::GetColumnNames() const
{
  std::cout << "GetColumnNames()" << std::endl;
  return m_columns;
}

std::vector<std::pair<ULong64_t,ULong64_t>> RPythiaSource::GetEntryRanges()
{
  std::cout << "GetEntryRanges()" << std::endl;
  std::vector<std::pair<ULong64_t,ULong64_t>> ranges;
  if(m_ranged) return ranges;
  m_ranged=true;
  
  ULong64_t entriesPerSlot=m_nEvents/m_nSlots;
  for(ULong64_t i=0;i<m_nSlots;i++)
    ranges.push_back(std::make_pair(i*entriesPerSlot , (i+1)*entriesPerSlot));
  if(ranges.back().second!=m_nEvents)
    ranges.push_back(std::make_pair(ranges.back().second,m_nEvents));

  for(auto x : ranges)
    std::cout << x.first << ", " << x.second << std::endl;
  
  return ranges;
}

std::string RPythiaSource::GetTypeName(std::string_view column) const
{
  std::cout << "GetTypeName(" << column << ")" << std::endl;
  //return "ROOT::VecOps::RVec<float>";
  return "uint32_t";
}

bool RPythiaSource::HasColumn(std::string_view column) const
{
  std::cout << "HasColumn(" << column << ")" << std::endl;
  return !(std::find(m_columns.begin(), m_columns.end(), column)==m_columns.end());
}

void RPythiaSource::Initialise()
{
  std::cout << "Initialise()" << std::endl;
  m_pythias.resize(m_nSlots);
  for(unsigned int slot=0; slot<m_nSlots; slot++)
    {
      m_pythias[slot]=std::make_shared<Pythia8::Pythia>();
      m_pythias[slot]->readFile(m_cmnd);
      m_pythias[slot]->readString("Random:setSeed = On");
      m_pythias[slot]->readString("Random:seed = "+std::to_string(slot+1));
      m_pythias[slot]->init();
    }
}

bool RPythiaSource::SetEntry(unsigned int slot, ULong64_t entry)
{
  std::cout << "SetEntry(" << slot << ", " << entry << ")" << std::endl;
  if(!m_pythias[slot]->next())
    {
      std::cout << "Error!" << std::endl;
      return false;
    }

  uint32_t nPart=m_pythias[slot]->event.size();
  m_column_uint32["nparticles"][slot]=nPart;
  std::cout << &m_column_uint32["nparticles"][slot] << std::endl;

  std::cout << "NPART " << nPart << std::endl;
  ROOT::VecOps::RVec<float> vec_pt(nPart);
  for(uint32_t i=0; i<nPart; i++)
    {
      vec_pt[i]=m_pythias[slot]->event[i].pT();
    }

  //m_column_vector_float["particles_pt"][slot].push_back(vec_pt);
  std::cout << "DONE" << std::endl;
  
  return true;
}

void RPythiaSource::SetNSlots(unsigned int nSlots)
{
  std::cout << "nSlots = " << nSlots << std::endl;
  m_nSlots=nSlots;

  m_column_uint32["nparticles"].resize(m_nSlots);
  //m_column_vector_float["particles_pt"].resize(m_nSlots);
  m_columnsAddrs["nparticles"].resize(m_nSlots);
  for(unsigned int slot=0; slot<m_nSlots; slot++)
    {
      m_columnsAddrs["nparticles"][slot]=&m_column_uint32["nparticles"][slot];
      std::cout << &m_column_uint32["nparticles"][slot] << std::endl;
      std::cout << "nparticles = " << m_columnsAddrs["nparticles"][slot] << std::endl;
    }
}

std::vector<void*> RPythiaSource::GetColumnReadersImpl(std::string_view column, const std::type_info &)
{
  std::cout << m_nSlots << std::endl;
  std::cout << "GetColumnReadersImpl(" << column << ", " << ")" << std::endl;
  std::vector<void *> ret(m_nSlots);

  for(unsigned int slot=0; slot<m_nSlots; slot++)
    {
      //m_column_vector_float["particles_pt"][slot].resize(100);
      std::cout << m_columnsAddrs["nparticles"][slot] << std::endl;
      ret[slot]=&m_columnsAddrs["nparticles"][slot];//&m_column_vector_float["particles_pt"][slot];
      std::cout << slot << ": " << ret[slot] << std::endl;
    }

  return ret; //&m_columnsAddrs["particles_pt"]; //ret;
}
