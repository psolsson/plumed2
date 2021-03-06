/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   Copyright (c) 2012 The plumed team
   (see the PEOPLE file at the root of the distribution for a list of names)

   See http://www.plumed-code.org for more information.

   This file is part of plumed, version 2.0.

   plumed is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   plumed is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with plumed.  If not, see <http://www.gnu.org/licenses/>.
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
#include "vesselbase/VesselRegister.h"
#include "vesselbase/FunctionVessel.h"
#include "Mapping.h"

namespace PLMD {
namespace mapping {

class ZpathVessel : public vesselbase::FunctionVessel {
private:
  double invlambda;
public:
  static void registerKeywords( Keywords& keys );
  static void reserveKeyword( Keywords& keys );
  ZpathVessel( const vesselbase::VesselOptions& da );
  std::string function_description();
  bool calculate();
  void finish();
};

PLUMED_REGISTER_VESSEL(ZpathVessel,"ZPATH")

void ZpathVessel::registerKeywords( Keywords& keys ){
  FunctionVessel::registerKeywords(keys);
}

void ZpathVessel::reserveKeyword( Keywords& keys ){
  keys.reserveFlag("ZPATH",false,"calculate the distance from the low dimensionality manifold",true);
}

ZpathVessel::ZpathVessel( const vesselbase::VesselOptions& da ):
FunctionVessel(da)
{
  Mapping* mymap=dynamic_cast<Mapping*>( getAction() );
  plumed_massert( mymap, "ZpathVessel should only be used with mappings");
  invlambda = 1.0 / mymap->getLambda();
}

std::string ZpathVessel::function_description(){
  return "the distance from the low-dimensional manifold";
}

bool ZpathVessel::calculate(){
  double weight=getAction()->getElementValue(0);
  bool addval=addValueUsingTolerance( 0, weight );
  if( addval ) getAction()->chainRuleForElementDerivatives( 0, 0, 1.0, this );
  return ( weight>getNLTolerance() );
}

void ZpathVessel::finish(){
  double sum = getFinalValue(0); std::vector<double> df(2);
  setOutputValue( -invlambda*std::log( sum ) );
  df[0] = -invlambda / sum; df[1] = 0.0;
  mergeFinalDerivatives( df );
}

}
}
