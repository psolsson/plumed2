/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   Copyright (c) 2013 The plumed team
   (see the PEOPLE file at the root of the distribution for a list of names)

   See http://www.plumed-code.org for more information.

   This file is part of plumed, version 2.

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
#include "core/ActionWithValue.h"
#include "core/ActionRegister.h"
#include "core/PlumedMain.h"
#include "core/ActionSet.h"
#include "vesselbase/BridgeVessel.h"
#include "vesselbase/ActionWithVessel.h"
#include "vesselbase/ActionWithInputVessel.h"
#include "VectorMultiColvar.h"

//+PLUMEDOC MCOLVARF AVERAGE_VECTOR
/*
Calculate the average vector by averaging each component of the vector separately

This colvar takes a set of atom centered vectors and calculates the average vector.  Unlike
the average calculated by using the following command:

\verbatim
Q6 SPECIES=1-64 D_0=1.3 R_0=0.2 MEAN LABEL=q6
\endverbatim

which calculates the average value for the norm of the Q6 vectors, the AVERAGE_VECTOR command takes
the average by averaging each component of the vector in turn.  This sort of thing has is sometimes 
used to measure the average Q4 parameter in a cluster. 

\par Examples

The following command calculates the average Q6 vector for a 75 atom cluster of Lennard Jones.  The
average is then printed to a file called colvar.

\verbatim
Q6 SPECIES=1-75 SWITCH={GAUSSIAN D_0=1.391 R_0=0.01} LABEL=q6
AVERAGE_VECTOR DATA=q6 LABEL=a6a
PRINT ARG=a6a FILE=colvar
\endverbatim

*/
//+ENDPLUMEDOC

namespace PLMD {
namespace crystallization {

class VectorAverage : 
  public PLMD::ActionWithValue,
  public vesselbase::ActionWithVessel,
  public vesselbase::ActionWithInputVessel
{
private:
  vesselbase::BridgeVessel* myBridgeVessel;
  VectorMultiColvar* mycolv;
  std::vector<double> values, derivatives;
public:
  static void registerKeywords( Keywords& keys );
  VectorAverage(const ActionOptions&ao);
/// Don't actually clear the derivatives when this is called from plumed main.  
/// They are calculated inside another action and clearing them would be bad  
  void clearDerivatives(){}
  void doJobsRequiredBeforeTaskList();
  unsigned getNumberOfDerivatives();
  void turnOnDerivatives();
  void performTask();
  void finishComputations();
  void calculate(){}
  void apply();
  bool isPeriodic();
  void deactivate_task();
};

PLUMED_REGISTER_ACTION(VectorAverage,"AVERAGE_VECTOR")

void VectorAverage::registerKeywords( Keywords& keys ){
  Action::registerKeywords( keys );
  ActionWithValue::registerKeywords( keys );
  ActionWithVessel::registerKeywords( keys );
  ActionWithInputVessel::registerKeywords( keys );
}

VectorAverage::VectorAverage(const ActionOptions&ao):
Action(ao),
ActionWithValue(ao),
ActionWithVessel(ao),
ActionWithInputVessel(ao)
{
  readArgument("bridge");
  mycolv = dynamic_cast<crystallization::VectorMultiColvar*>( getDependencies()[0] );
  plumed_assert( getDependencies().size()==1 );
  if(!mycolv) error("action labeled " + mycolv->getLabel() + " does not exist or is not a multicolvar");
  std::string functype=mycolv->getName();
  log.printf("  calculating average %s vector\n",functype.c_str() );

  // And create value
  addValueWithDerivatives(); setNotPeriodic(); 
  // Resize everything
  unsigned nder = mycolv->getNumberOfDerivatives();
  unsigned ncomp = mycolv->getNumberOfQuantities() - 5;
  values.resize( ncomp ); derivatives.resize( ncomp*nder );
  getPntrToComponent(0)->resizeDerivatives( nder );
}

void VectorAverage::doJobsRequiredBeforeTaskList(){
  ActionWithValue::clearDerivatives(); 
  values.assign( values.size(), 0.0 );
  derivatives.assign( derivatives.size(), 0.0 );
}

void VectorAverage::turnOnDerivatives(){
  ActionWithValue::turnOnDerivatives();
  needsDerivatives();
}

unsigned VectorAverage::getNumberOfDerivatives(){
  return mycolv->getNumberOfDerivatives();
}

void VectorAverage::performTask(){
  unsigned nder=mycolv->getNumberOfDerivatives();
  for(unsigned i=5;i<mycolv->getNumberOfQuantities();++i){
     values[i-5] += mycolv->getElementValue( i ); 
     unsigned nl=(i-5)*nder, nj=i*nder;
     for(unsigned j=0;j<mycolv->getNumberOfDerivatives();++j){
         derivatives[nl+j] += mycolv->getElementDerivative( nj + j );
     }
  }
}

void VectorAverage::finishComputations(){
  comm.Sum( values ); comm.Sum( derivatives ); 
  double norm = static_cast<double>( mycolv->getFullNumberOfTasks() );
  double sum=0; unsigned nder = mycolv->getNumberOfDerivatives();
  for(unsigned i=0;i<values.size();++i){ values[i]/=norm; sum+=values[i]*values[i]; }

  double inorm = 1.0 / ( norm*sqrt(sum) );
  Value* val=getPntrToComponent(0); val->set( sqrt(sum) ); 
  for(unsigned icomp=0;icomp<values.size();++icomp){
     for(unsigned jder=0;jder<mycolv->getNumberOfDerivatives();++jder){
        val->addDerivative( jder, inorm*values[icomp]*derivatives[nder*icomp+jder] );
     }
  }
}

void VectorAverage::apply(){
  Value* val=getPntrToComponent(0); 
  std::vector<double> tforces( mycolv->getNumberOfDerivatives(), 0 );
  if( val->applyForce( tforces ) ) mycolv->addForcesOnAtoms( tforces );
}

bool VectorAverage::isPeriodic(){
  plumed_merror("This should never be called");
  return mycolv->isPeriodic();
}

void VectorAverage::deactivate_task(){
  plumed_merror("This should never be called");
}


}
}
