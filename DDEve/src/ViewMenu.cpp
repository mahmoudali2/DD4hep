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
#include <DD4hep/Plugins.h>
#include <DD4hep/Printout.h>
#include <DD4hep/InstanceCount.h>

#include <DDEve/View.h>
#include <DDEve/ViewMenu.h>
#include <DDEve/PopupMenu.h>

// ROOT include files
#include <TEveBrowser.h>
#include <TEveManager.h>
#include <TEveElement.h>
#include <TEveWindow.h>
#include <TEveViewer.h>
#include <TGLViewer.h>
#include <TGClient.h>
#include <TGTab.h>

// Forward declarations
class TEveWindowSlot;

using namespace dd4hep;
using pair_t = std::pair<std::string, std::string>;

ClassImp(ViewMenu)

/// Initializing constructor
ViewMenu::ViewMenu(Display* display, const std::string& title) 
: PopupMenu(display->client().GetRoot()), m_display(display), m_title(title)
{
  InstanceCount::increment(this);
}

/// Default destructor
ViewMenu::~ViewMenu()  {
  InstanceCount::decrement(this);
}

/// Add the menu to the menu bar
void ViewMenu::Build(TGMenuBar* menubar, int hints)    {
  pair_t* p = 0;
  PopupMenu* view_menu = this;
  view_menu->AddEntry("3&D View", this, &ViewMenu::CreateView, p=new pair_t("DD4hep_DDEve_View3D","3D"));
  view_menu->AddEntry("Rho-&Z Projection", this, &ViewMenu::CreateView, p=new pair_t("DD4hep_DDEve_RhoZProjection","Rho-Z"));
  view_menu->AddEntry("Rho-&Phi Projection",this, &ViewMenu::CreateView, p=new pair_t("DD4hep_DDEve_RhoPhiProjection","Rho-Phi"));
  const Display::ViewConfigurations& vc = m_display->viewConfigurations();
  for(Display::ViewConfigurations::const_iterator i=vc.begin(); i!=vc.end(); ++i)  {
    const Display::ViewConfig& v = (*i).second;
    view_menu->AddEntry(v.name.c_str(), this, &ViewMenu::CreateView,p=new pair_t("DD4hep_DDEve_"+v.type,v.name));
  }
  menubar->AddPopup(m_title.c_str(),*view_menu, new TGLayoutHints(hints, 0, 4, 0, 0));
}

/// Create a new generic view
void ViewMenu::CreateView(TGMenuEntry*, void* ud)   {
  pair_t* args = (pair_t*)ud;
  CreateView(args->first,args->second);
}

View* ViewMenu::CreateView(const std::string& type, const std::string& title)   {
  pair_t args("DD4hep_DDEve_"+type,title);
  View* v = PluginService::Create<View*>(type.c_str(),m_display,title.c_str());
  BuildView(v);
  return v;
}

/// Create a new 3D view
View* ViewMenu::CreateView3D(const std::string& title)   {
  return CreateView("DD4hep_DDEve_View3D",title.c_str());
}

/// Create a new R-Z view
View* ViewMenu::CreateRhoZProjection(const std::string& title )  {
  return CreateView("DD4hep_DDEve_RhoZProjection",title.c_str());
}

/// Create a new R-Phi view
View* ViewMenu::CreateRhoPhiProjection(const std::string& title )  {
  return CreateView("DD4hep_DDEve_RhoPhiProjection",title.c_str());
}

/// Import Geometry data
void ViewMenu::BuildView(View* v)  const  {
  TEveManager& mgr = m_display->manager();
  TEveBrowser    *browser = mgr.GetBrowser();
  TGTab          *right = browser->GetTabRight();
  TEveWindowSlot *slot = TEveWindow::CreateWindowInTab(right);
  v->Build(slot);
  m_display->RegisterEvents(v);
  v->ConfigureGeometryFromInfo();
  v->ConfigureEventFromInfo();
  v->Initialize();
  right->SetTab(right->GetNumberOfTabs()-1);
}
