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

#ifndef DD4HEP_DDG4_GEANT4FIELDTRACKINGSETUP_H
#define DD4HEP_DDG4_GEANT4FIELDTRACKINGSETUP_H 1

// Framework include files
#include <DD4hep/Detector.h>
#include <DDG4/Geant4ActionPhase.h>
#include <DDG4/Geant4DetectorConstruction.h>

/// Namespace for the AIDA detector description toolkit
namespace dd4hep {

  /// Namespace for the Geant4 based simulation part of the AIDA detector description toolkit
  namespace sim {

    /// Generic Setup component to perform the magnetic field tracking in Geant4
    /** Geant4FieldTrackingSetup.
     *
     *  This base class is use jointly between the XML setup and the 
     *  phase action used by the python setup.
     *
     *  Note:
     *  Negative parameters are not passed to Geant4 objects, but ignored -- if possible.
     *
     * @author  M.Frank
     * @version 1.0
     */
    struct Geant4FieldTrackingSetup  {
    protected:
      /** Variables to be filled before calling execute */
      /// Name of the G4Mag_EqRhs class
      std::string eq_typ;
      /// Name of the G4MagIntegratorStepper class
      std::string stepper_typ;
      /// G4ChordFinder parameter: min_chord_step
      double      min_chord_step;
      /// G4ChordFinder parameter: delta
      double      delta_chord;
      /// G4FieldManager parameter: delta_one_step
      double      delta_one_step;
      /// G4FieldManager parameter: delta_intersection
      double      delta_intersection;
      /// G4PropagatorInField parameter: eps_min
      double      eps_min;
      /// G4PropagatorInField parameter: eps_min
      double      eps_max;
      /// G4PropagatorInField parameter: LargestAcceptableStep
      double      largest_step;

    public:
      /// Default constructor
      Geant4FieldTrackingSetup();
      /// Default destructor
      virtual ~Geant4FieldTrackingSetup();
      /// Perform the setup of the magnetic field tracking in Geant4
      virtual int execute(Detector& description);
    };

    /// Phase action to perform the setup of the Geant4 tracking in magnetic fields
    /** Geant4FieldTrackingSetupAction.
     *
     *  The phase action configures the Geant4FieldTrackingSetup base class using properties
     *  and then configures the Geant4 tracking in magnetic fields.
     *
     * @author  M.Frank
     * @version 1.0
     */
    class Geant4FieldTrackingSetupAction :
      public Geant4PhaseAction,
      public Geant4FieldTrackingSetup
    {
    protected:
    public:
      /// Standard constructor
      Geant4FieldTrackingSetupAction(Geant4Context* context, const std::string& nam);
      /// Default destructor
      virtual ~Geant4FieldTrackingSetupAction() {}
      /// Phase action callback
      void operator()();
    };

    /// Detector construction action to perform the setup of the Geant4 tracking in magnetic fields
    /** Geant4FieldTrackingSetupAction.
     *
     *  The phase action configures the Geant4FieldTrackingSetup base class using properties
     *  and then configures the Geant4 tracking in magnetic fields.
     *
     * @author  M.Frank
     * @version 1.0
     */
    class Geant4FieldTrackingConstruction : 
      public Geant4DetectorConstruction,
      public Geant4FieldTrackingSetup
    {
    protected:
    public:
      /// Standard constructor
      Geant4FieldTrackingConstruction(Geant4Context* context, const std::string& nam);

      /// Default destructor
      virtual ~Geant4FieldTrackingConstruction() {}

      /// Detector construction callback
      void constructField(Geant4DetectorConstructionContext *ctxt);

    };
  }    // End namespace sim
}      // End namespace dd4hep
#endif // DD4HEP_DDG4_GEANT4FIELDTRACKINGSETUP_H

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
#include <DD4hep/Handle.h>
#include <DD4hep/Fields.h>
#include <DDG4/Factories.h>
#include <DDG4/Geant4Field.h>
#include <DDG4/Geant4Converter.h>

#include <G4TransportationManager.hh>
#include <G4MagIntegratorStepper.hh>
#include <G4Mag_EqRhs.hh>
#include <G4ChordFinder.hh>
#include <G4PropagatorInField.hh>
#include <limits>

using namespace dd4hep::sim;

/// Local declaration in anonymous namespace
namespace {

  struct Geant4SetupPropertyMap {
    const std::map<std::string,std::string>& vals;
    Geant4SetupPropertyMap(const std::map<std::string,std::string>& v) : vals(v) {}
    std::string value(const std::string& key) const;
    double toDouble(const std::string& key) const;
    bool operator[](const std::string& key) const  { return vals.find(key) != vals.end(); }
  };
  
  std::string Geant4SetupPropertyMap::value(const std::string& key) const {
    dd4hep::Detector::PropertyValues::const_iterator iV = vals.find(key);
    return iV == vals.end() ? "" : (*iV).second;
  }

  double Geant4SetupPropertyMap::toDouble(const std::string& key) const {
    return dd4hep::_toDouble(this->value(key));
  }

}

/// Default constructor
Geant4FieldTrackingSetup::Geant4FieldTrackingSetup() : eq_typ(), stepper_typ() {
  eps_min            = -1.0;
  eps_max            = -1.0;
  min_chord_step     =  1.0e-2 *CLHEP::mm;
  delta_chord        = -1.0;
  delta_one_step     = -1.0;
  delta_intersection = -1.0;
  largest_step       = -1.0;
}

/// Default destructor
Geant4FieldTrackingSetup::~Geant4FieldTrackingSetup()   {
}

/// Perform the setup of the magnetic field tracking in Geant4
int Geant4FieldTrackingSetup::execute(Detector& description)   {
  OverlayedField fld  = description.field();
  G4ChordFinder*           chordFinder;
  G4TransportationManager* transportMgr;
  G4PropagatorInField*     propagator;
  G4FieldManager*          fieldManager;
  G4MagneticField*         mag_field    = new sim::Geant4Field(fld);
  G4Mag_EqRhs*             mag_equation = PluginService::Create<G4Mag_EqRhs*>(eq_typ,mag_field);
  G4EquationOfMotion*      mag_eq       = mag_equation;
  if ( nullptr == mag_eq )   {
    mag_eq = PluginService::Create<G4EquationOfMotion*>(eq_typ,mag_field);
    if ( nullptr == mag_eq )   {
      except("FieldSetup", "Cannot create G4EquationOfMotion of type: %s.",eq_typ.c_str());
    }
  }
  G4MagIntegratorStepper* fld_stepper = PluginService::Create<G4MagIntegratorStepper*>(stepper_typ,mag_eq);
  if ( nullptr == fld_stepper )   {
    fld_stepper  = PluginService::Create<G4MagIntegratorStepper*>(stepper_typ,mag_equation);
    if ( nullptr == fld_stepper )   {
      except("FieldSetup", "Cannot create stepper of type: %s.",stepper_typ.c_str());
    }
  }
  chordFinder  = new G4ChordFinder(mag_field,min_chord_step,fld_stepper);
  transportMgr = G4TransportationManager::GetTransportationManager();
  propagator   = transportMgr->GetPropagatorInField();
  fieldManager = transportMgr->GetFieldManager();

  fieldManager->SetFieldChangesEnergy(fld.changesEnergy());
  fieldManager->SetDetectorField(mag_field);
  fieldManager->SetChordFinder(chordFinder);

  if ( delta_chord >= 0e0 )
    chordFinder->SetDeltaChord(delta_chord);
  if ( delta_one_step >= 0e0 )
    fieldManager->SetAccuraciesWithDeltaOneStep(delta_one_step);
  if ( delta_intersection >= 0e0 )
    fieldManager->SetDeltaIntersection(delta_intersection);
  if ( eps_min >= 0e0 )
    propagator->SetMinimumEpsilonStep(eps_min);
  if ( eps_max >= 0e0 )
    propagator->SetMaximumEpsilonStep(eps_max);
  if ( largest_step >= 0e0 ) {
    propagator->SetLargestAcceptableStep(largest_step);
  } else {
    largest_step = propagator->GetLargestAcceptableStep();
  }
  return 1;
}

static long setup_fields(dd4hep::Detector& description,
                         const dd4hep::detail::GeoHandler& /* cnv */,
                         const std::map<std::string,std::string>& vals)
{
  struct XMLFieldTrackingSetup : public Geant4FieldTrackingSetup {
    XMLFieldTrackingSetup(const std::map<std::string,std::string>& values) : Geant4FieldTrackingSetup() {
      Geant4SetupPropertyMap pm(values);
      dd4hep::Detector::PropertyValues::const_iterator iV = values.find("min_chord_step");
      eq_typ      = pm.value("equation");
      stepper_typ = pm.value("stepper");
      min_chord_step = dd4hep::_toDouble((iV==values.end()) ? std::string("1e-2 * mm") : (*iV).second);
      if ( pm["eps_min"] ) eps_min = pm.toDouble("eps_min");
      if ( pm["eps_max"] ) eps_max = pm.toDouble("eps_max");
      if ( pm["delta_chord"] ) delta_chord = pm.toDouble("delta_chord");
      if ( pm["delta_one_step"] ) delta_one_step = pm.toDouble("delta_one_step");
      if ( pm["delta_intersection"] ) delta_intersection = pm.toDouble("delta_intersection");
      if ( pm["largest_step"] ) largest_step = pm.toDouble("largest_step");
    }
    virtual ~XMLFieldTrackingSetup() {}
  } setup(vals);
  return setup.execute(description);
}

/// Standard constructor
Geant4FieldTrackingSetupAction::Geant4FieldTrackingSetupAction(Geant4Context* ctxt, const std::string& nam)
  : Geant4PhaseAction(ctxt,nam), Geant4FieldTrackingSetup()
{
  declareProperty("equation",           eq_typ);
  declareProperty("stepper",            stepper_typ);
  declareProperty("min_chord_step",     min_chord_step = 1.0e-2);
  declareProperty("delta_chord",        delta_chord = -1.0);
  declareProperty("delta_one_step",     delta_one_step = -1.0);
  declareProperty("delta_intersection", delta_intersection = -1.0);
  declareProperty("eps_min",            eps_min = -1.0);
  declareProperty("eps_max",            eps_max = -1.0);
  declareProperty("largest_step",       largest_step = -1.0);
}

/// Post-track action callback
void Geant4FieldTrackingSetupAction::operator()()   {
  execute(context()->detectorDescription());
  printout( INFO, "FieldSetup", "Geant4 magnetic field tracking configured.");
  printout( INFO, "FieldSetup", "G4MagIntegratorStepper:%s G4Mag_EqRhs:%s",
	    stepper_typ.c_str(), eq_typ.c_str());
  printout( INFO, "FieldSetup", "Epsilon:[min:%f mm max:%f mm]", eps_min, eps_max);
  printout( INFO, "FieldSetup", "Delta:[chord:%f 1-step:%f intersect:%f] LargestStep %f mm",
	    delta_chord, delta_one_step, delta_intersection, largest_step);
}


/// Standard constructor
Geant4FieldTrackingConstruction::Geant4FieldTrackingConstruction(Geant4Context* ctxt, const std::string& nam)
  : Geant4DetectorConstruction(ctxt,nam), Geant4FieldTrackingSetup()
{
  declareProperty("equation",           eq_typ);
  declareProperty("stepper",            stepper_typ);
  declareProperty("min_chord_step",     min_chord_step = 1.0e-2);
  declareProperty("delta_chord",        delta_chord = -1.0);
  declareProperty("delta_one_step",     delta_one_step = -1.0);
  declareProperty("delta_intersection", delta_intersection = -1.0);
  declareProperty("eps_min",            eps_min = -1.0);
  declareProperty("eps_max",            eps_max = -1.0);
  declareProperty("largest_step",       largest_step = -1.0);
}

/// Detector construction callback
void Geant4FieldTrackingConstruction::constructField(Geant4DetectorConstructionContext *) {
  execute(context()->detectorDescription());
  printout( INFO, "FieldSetup", "Geant4 magnetic field tracking configured.");
  printout( INFO, "FieldSetup", "G4MagIntegratorStepper:%s G4Mag_EqRhs:%s",
	    stepper_typ.c_str(), eq_typ.c_str());
  printout( INFO, "FieldSetup", "Epsilon:[min:%f mm max:%f mm]", eps_min, eps_max);
  printout( INFO, "FieldSetup", "Delta:[chord:%f 1-step:%f intersect:%f] LargestStep %f mm",
	    delta_chord, delta_one_step, delta_intersection, largest_step);
}

DECLARE_GEANT4_SETUP(Geant4FieldSetup,setup_fields)
DECLARE_GEANT4ACTION(Geant4FieldTrackingSetupAction)
DECLARE_GEANT4ACTION(Geant4FieldTrackingConstruction)
