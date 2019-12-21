#include "RPythiaSource.h"

#include <iostream>

RPythiaSource::RPythiaSource(const std::string& cmnd, ULong64_t nEvents)
  : m_cmnd(cmnd), m_nEvents(nEvents)
{ }

const std::vector<std::string>& RPythiaSource::GetColumnNames() const
{
  return m_columns;
}

std::vector<std::pair<ULong64_t,ULong64_t>> RPythiaSource::GetEntryRanges()
{
  std::vector<std::pair<ULong64_t,ULong64_t>> ranges;
  if(m_ranged) return ranges;
  m_ranged=true;
  
  ULong64_t entriesPerSlot=m_nEvents/m_nSlots;
  for(ULong64_t i=0;i<m_nSlots;i++)
    ranges.push_back(std::make_pair(i*entriesPerSlot , (i+1)*entriesPerSlot));
  if(ranges.back().second!=m_nEvents)
    ranges.push_back(std::make_pair(ranges.back().second,m_nEvents));
  
  return ranges;
}

std::string RPythiaSource::GetTypeName(std::string_view column) const
{
  return m_columnsTypes.at(column.data());
}

bool RPythiaSource::HasColumn(std::string_view column) const
{
  return !(std::find(m_columns.begin(), m_columns.end(), column)==m_columns.end());
}

void RPythiaSource::Initialise()
{
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
  if(!m_pythias[slot]->next())
    return false;

  uint32_t nPart=m_pythias[slot]->event.size();
  m_column_uint32["nparticles"][slot]=nPart;

  m_column_vector_float["particles_pt"    ][slot].resize(nPart);
  m_column_vector_float["particles_eta"   ][slot].resize(nPart);
  m_column_vector_float["particles_phi"   ][slot].resize(nPart);
  m_column_vector_float["particles_m"     ][slot].resize(nPart);
  m_column_vector_int32["particles_pdg"   ][slot].resize(nPart);
  m_column_vector_int32["particles_status"][slot].resize(nPart);
  for(uint32_t i=0; i<nPart; i++)
    {
      m_column_vector_float["particles_pt"    ][slot][i]=m_pythias[slot]->event[i].pT    ();
      m_column_vector_float["particles_eta"   ][slot][i]=m_pythias[slot]->event[i].eta   ();
      m_column_vector_float["particles_phi"   ][slot][i]=m_pythias[slot]->event[i].phi   ();
      m_column_vector_float["particles_m"     ][slot][i]=m_pythias[slot]->event[i].m     ();
      m_column_vector_int32["particles_pdg"   ][slot][i]=m_pythias[slot]->event[i].id    ();
      m_column_vector_int32["particles_status"][slot][i]=m_pythias[slot]->event[i].status();
    }
  
  return true;
}

void RPythiaSource::SetNSlots(unsigned int nSlots)
{
  m_nSlots=nSlots;

  m_column_uint32      ["nparticles"      ].resize(m_nSlots);
  m_column_vector_float["particles_pt"    ].resize(m_nSlots);
  m_column_vector_float["particles_eta"   ].resize(m_nSlots);
  m_column_vector_float["particles_phi"   ].resize(m_nSlots);
  m_column_vector_float["particles_m"     ].resize(m_nSlots);
  m_column_vector_int32["particles_pdg"   ].resize(m_nSlots);
  m_column_vector_int32["particles_status"].resize(m_nSlots);

  for(std::pair<std::string, std::vector<uint32_t>> kv : m_column_uint32)
    {
      m_columnsTypes[kv.first]="uint32_t";
      m_columnsAddrs[kv.first].resize(m_nSlots);
      for(unsigned int slot=0; slot<m_nSlots; slot++)
	m_columnsAddrs[kv.first][slot]=&m_column_uint32[kv.first][slot];
    }

  for(std::pair<std::string, std::vector<ROOT::VecOps::RVec<float>>> kv : m_column_vector_float)
    {
      m_columnsTypes[kv.first]="ROOT::VecOps::RVec<float>";
      m_columnsAddrs[kv.first].resize(m_nSlots);
      for(unsigned int slot=0; slot<m_nSlots; slot++)
	m_columnsAddrs[kv.first][slot]=&m_column_vector_float[kv.first][slot];
    }

  for(std::pair<std::string, std::vector<ROOT::VecOps::RVec<int32_t>>> kv : m_column_vector_int32)
    {
      m_columnsTypes[kv.first]="ROOT::VecOps::RVec<int32_t>";
      m_columnsAddrs[kv.first].resize(m_nSlots);
      for(unsigned int slot=0; slot<m_nSlots; slot++)
	m_columnsAddrs[kv.first][slot]=&m_column_vector_int32[kv.first][slot];
    }
}

std::vector<void*> RPythiaSource::GetColumnReadersImpl(std::string_view column, const std::type_info &)
{
  std::vector<void *> ret(m_nSlots);

  for(unsigned int slot=0; slot<m_nSlots; slot++)
    ret[slot]=&m_columnsAddrs[column.data()][slot];

  return ret;
}
