//==========================================================================
//  AIDA Detector description implementation 
//--------------------------------------------------------------------------
// Copyright (C) Organisation europeenne pour la Recherche nucleaire (CERN)
// All rights reserved.
//
// For the licensing terms see $DD4hepINSTALL/LICENSE.
// For the list of contributors see $DD4hepINSTALL/doc/CREDITS.
//
// Author     : M.Frank
//
//==========================================================================

// Framework include files
#include "DDCond/ConditionsTextRepository.h"
#include "DDCond/ConditionsIOVPool.h"
#include "DDCond/ConditionsTags.h"
#include "DD4hep/Printout.h"
#include "DD4hep/detail/ConditionsInterna.h"
#include "XML/DocumentHandler.h"
#include "XML/XMLTags.h"

// C/C++ include files
#include <cstring>
#include <fstream>
#include <climits>
#include <cerrno>
#include <map>

using namespace dd4hep::cond;

typedef dd4hep::xml::Handle_t     xml_h;
typedef dd4hep::xml::Element      xml_elt_t;
typedef dd4hep::xml::Document     xml_doc_t;
typedef dd4hep::xml::Collection_t xml_coll_t;

typedef std::map<dd4hep::Condition::key_type,dd4hep::Condition> AllConditions;

/// Default constructor. Allocates resources
ConditionsTextRepository::ConditionsTextRepository()  {
}

/// Default destructor
ConditionsTextRepository::~ConditionsTextRepository()   {
}

namespace {

  int createXML(const std::string& output, const AllConditions& all) {
    char text[32];
    const char comment[] = "\n"
      "      +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n"
      "      ++++   Linear collider detector description Detector in C++  ++++\n"
      "      ++++   dd4hep Detector description generator.            ++++\n"
      "      ++++                                                     ++++\n"
      "      ++++                                                     ++++\n"
      "      ++++                              M.Frank CERN/LHCb      ++++\n"
      "      +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n  ";
    dd4hep::xml::DocumentHandler docH;
    xml_doc_t doc = docH.create("collection", comment);
    xml_elt_t root = doc.root(), cond(0);
    for( const auto& c : all )  {
      ::snprintf(text,sizeof(text),"0x%16llX",c.second.key());
      root.append(cond = xml_elt_t(doc, _U(ref)));
      cond.setAttr(_U(key),  text);
      cond.setAttr(_U(name), c.second.name());
#if !defined(DD4HEP_MINIMAL_CONDITIONS)
      cond.setAttr(_U(ref),  c.second.address());
#endif
    }
    dd4hep::printout(dd4hep::ALWAYS,"ConditionsRepository","++ Handled %ld conditions.",all.size());
    if ( !output.empty() )  {
      return docH.output(doc, output);
    }
    return 1;
  }

  /// Load the repository from file and fill user passed data structory
  int readXML(const std::string& input, ConditionsTextRepository::Data& data)    {
    struct Conv {
      /// Reference to optional user defined parameter
      ConditionsTextRepository::Data& data;
      /// Initializing constructor of the functor
      Conv(ConditionsTextRepository::Data& p) : data(p) {}
      /// Callback operator to be specialized depending on the element type
      void operator()(xml_h element) const   {
        std::string key = element.attr<std::string>(_U(key));
        size_t cap = data.capacity();
        ConditionsTextRepository::Entry e;
        ::sscanf(key.c_str(),"0x%16llX",&e.key);
        e.name = element.attr<std::string>(_U(name));
        e.address = element.attr<std::string>(_U(ref));
        if ( data.size() == cap ) data.reserve(cap+500);
        data.emplace_back(e);
      }
    };
    dd4hep::xml::DocumentHolder doc(dd4hep::xml::DocumentHandler().load(input));
    xml_h root = doc.root();
    xml_coll_t(root, _U(ref)).for_each(Conv(data));
    return 1;
  }
  
#if defined(DD4HEP_MINIMAL_CONDITIONS)
  int createText(const std::string& output, const AllConditions&, char)
#else
  int createText(const std::string& output, const AllConditions& all, char sep)
#endif
  {
    std::ofstream out(output);
#if !defined(DD4HEP_MINIMAL_CONDITIONS)
    std::size_t siz_nam=0, siz_add=0, siz_tot=0;
    char fmt[64], text[2*PATH_MAX+64];
    if ( !out.good() )  {
      dd4hep::except("ConditionsTextRepository",
                     "++ Failed to open output file:%s [errno:%d %s]",
                     output.c_str(), errno, ::strerror(errno));
    }
    else if ( sep )  {
      std::snprintf(fmt,sizeof(fmt),"%%16llX%c%%s%c%%s%c",sep,sep,sep);
    }
    else   {
      for( const auto& i : all )  {
        dd4hep::Condition::Object* c = i.second.ptr();
        std::size_t siz_n = c->name.length();
        std::size_t siz_a = c->address.length();
        if ( siz_add < siz_a ) siz_add = siz_a;
        if ( siz_nam < siz_n ) siz_nam = siz_n;
        if ( siz_tot < (siz_n+siz_a) ) siz_tot = siz_n+siz_a;
      }
      siz_tot += 8+2+1;
      ::snprintf(fmt,sizeof(fmt),"%%16llX %%-%lds %%-%lds",long(siz_nam),long(siz_add));
    }
    out << "dd4hep." << char(sep ? sep : '-')
        << "." << long(siz_nam)
        << "." << long(siz_add)
        << "." << long(siz_tot) << std::endl;
    for( const auto& i : all )  {
      dd4hep::Condition c = i.second;
      std::snprintf(text, sizeof(text), fmt, c.key(), c.name(), c.address().c_str());
      out << text << std::endl;
    }
#endif
    out.close();
    return 1;
  }

  /// Load the repository from file and fill user passed data structory
  int readText(const std::string& input, ConditionsTextRepository::Data& data)    {
    std::size_t idx;
    ConditionsTextRepository::Entry e;
    std::size_t siz_nam, siz_add, siz_tot;
    char sep, c, text[2*PATH_MAX+64];
    std::ifstream in(input);
    in >> c >> c >> c >> c >> c >> c >> c >> sep 
       >> c >> siz_nam
       >> c >> siz_add
       >> c >> siz_tot;
    text[0] = 0;
    in.getline(text,sizeof(text),'\n');
    do {
      text[0] = 0;
      in.getline(text,sizeof(text),'\n');
      if ( in.good() )  {
        std::size_t idx_nam = 9+siz_nam<sizeof(text)-1 ? 9+siz_nam : 0;
        std::size_t idx_add = 10+siz_nam+siz_add<sizeof(text)-1 ? 10+siz_nam+siz_add : 0;
        if ( 9+siz_nam >= sizeof(text) )
          dd4hep::except("ConditionsTextRepository","Inconsistent input data in %s: %s -> (%lld,%lld,%lld)",
                         __FILE__, input.c_str(), siz_nam, siz_add, siz_tot);
        else if ( 10+siz_nam+siz_add >= sizeof(text) )
          dd4hep::except("ConditionsTextRepository","Inconsistent input data in %s: %s -> (%lld,%lld,%lld)",
                         __FILE__, input.c_str(), siz_nam, siz_add, siz_tot);
        else if ( siz_tot )  {
          // Direct access mode with fixed record size
          text[8]   = text[idx_nam] = text[idx_add] = 0;
          e.name    = text+9;
          e.address = text+idx_nam+1;  
          if ( (idx=e.name.find(' ')) != std::string::npos )
            e.name[idx]=0;
          if ( (idx=e.address.find(' ')) != std::string::npos )
            e.address[idx]=0;
        }
        else  {
          // Variable record size
          e.name=text+9;
          if ( (idx=e.name.find(sep)) != std::string::npos && idx+10 < sizeof(text) )
            text[9+idx]=0, e.address=text+idx+10, e.name=text+9;
          if ( (idx=e.address.find(sep)) != std::string::npos )
            e.address[idx]=0;
          else if ( (idx=e.address.find('\n')) != std::string::npos )
            e.address[idx]=0;
        }
        std::size_t cap = data.capacity();
        std::sscanf(text,"%16llX",&e.key);
        if ( data.size() == cap ) data.reserve(cap+500);
        data.emplace_back(e);
      }
    } while(in.good() && !in.eof() );
    in.close();
    return 1;
  }
}

/// Save the repository to file
int ConditionsTextRepository::save(ConditionsManager manager, const std::string& output)  const  {
  AllConditions all;
  const auto types = manager.iovTypesUsed();
  for( const IOVType* type : types )  {
    if ( type )   {
      ConditionsIOVPool* pool = manager.iovPool(*type);
      if ( pool )  {
        for ( const auto& cp : pool->elements )   {
          RangeConditions rc;
          cp.second->select_all(rc);
          for(auto c : rc )
            all[c.key()] = c;
        }
      }
    }
  }

  if ( output.find(".xml") != std::string::npos )   {
    /// Write XML file with conditions addresses
    return createXML(output, all);
  }
  else if ( output.find(".txt") != std::string::npos )   {
    /// Write fixed records with conditions addresses
    return createText(output, all, 0);
  }
  else if ( output.find(".daf") != std::string::npos )   {
    /// Write fixed records with conditions addresses
    return createText(output, all, 0);
  }
  else if ( output.find(".csv") != std::string::npos )   {
    /// Write records separated by ';' with conditions addresses
    return createText(output, all, ';');
  }
  return 0;
}

/// Load the repository from file and fill user passed data structory
int ConditionsTextRepository::load(const std::string& input, Data& data)  const  {
  if ( input.find(".xml") != std::string::npos )   {
    return readXML(input, data);
  }
  else if ( input.find(".txt") != std::string::npos )   {
    return readText(input, data);
  }
  else if ( input.find(".daf") != std::string::npos )   {
    return readText(input, data);
  }
  else if ( input.find(".csv") != std::string::npos )   {
    return readText(input, data);
  }
  return 0;
}
