#ifndef RPYTHIASOURCE_H
#define RPYTHIASOURCE_H

#include <ROOT/RDataSource.hxx>
#include <ROOT/RVec.hxx>

#include <Pythia8/Pythia.h>

#include <unordered_map>

class RPythiaSource : public ROOT::RDF::RDataSource
{
public:
  RPythiaSource(const std::string& cmnd, ULong64_t nEvents);
  virtual ~RPythiaSource() =default;

  virtual const std::vector<std::string>& GetColumnNames() const;

  virtual std::vector<std::pair<ULong64_t,ULong64_t>> GetEntryRanges();

  virtual std::string GetTypeName(std::string_view column) const;

  virtual bool HasColumn(std::string_view column) const;

  virtual void Initialise();
  virtual bool SetEntry(unsigned int slot, ULong64_t entry);
  virtual void SetNSlots(unsigned int nSlots);

protected:
  virtual std::vector<void*> GetColumnReadersImpl(std::string_view name, const std::type_info &);

private:
  std::string m_cmnd;
  ULong64_t m_nEvents;
  
  unsigned int m_nSlots=1;
  bool m_ranged=false;

  std::vector<std::string> m_columns = {"nparticles", "particles_pt", "particles_eta", "particles_phi", "particles_m", "particles_pdg", "particles_status"};
  std::vector<std::shared_ptr<Pythia8::Pythia>> m_pythias;

  std::unordered_map<std::string, std::vector<void*>> m_columnsAddrs;
  std::unordered_map<std::string, std::string>        m_columnsTypes;
  std::unordered_map<std::string, std::vector<float>> m_column_float;
  std::unordered_map<std::string, std::vector<uint32_t>> m_column_uint32;
  std::unordered_map<std::string, std::vector<ROOT::VecOps::RVec<float  >>> m_column_vector_float;
  std::unordered_map<std::string, std::vector<ROOT::VecOps::RVec<int32_t>>> m_column_vector_int32;
};

#endif // RPYTHIASOURCE_H
