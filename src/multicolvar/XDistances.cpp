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
#include "MultiColvar.h"
#include "core/ActionRegister.h"

#include <string>
#include <cmath>

using namespace std;

namespace PLMD{
namespace multicolvar{

//+PLUMEDOC MCOLVAR DISTANCES
/*
Calculate the distances between one or many pairs of atoms.  You can then calculate functions of the distribution of
distances such as the minimum, the number less than a certain quantity and so on. 

\par Examples

The following input tells plumed to calculate the distances between atoms 3 and 5 and between atoms 1 and 2 and to 
print the minimum for these two distances.
\verbatim
DISTANCES ATOMS1=3,5 ATOMS2=1,2 MIN={BETA=0.1} LABEL=d1
PRINT ARG=d1.min
\endverbatim
(See also \ref PRINT).

The following input tells plumed to calculate the distances between atoms 3 and 5 and between atoms 1 and 2
and then to calculate the number of these distances that are less than 0.1 nm.  The number of distances
less than 0.1nm is then printed to a file.
\verbatim
DISTANCES ATOMS1=3,5 ATOMS2=1,2 LABEL=d1 LESS_THAN={RATIONAL R_0=0.1}
PRINT ARG=d1.lt0.1
\endverbatim
(See also \ref PRINT \ref switchingfunction).

The following input tells plumed to calculate all the distances between atoms 1, 2 and 3 (i.e. the distances between atoms
1 and 2, atoms 1 and 3 and atoms 2 and 3).  The average of these distances is then calculated.
\verbatim
DISTANCES GROUP=1-3 AVERAGE LABEL=d1
PRINT ARG=d1.average
\endverbatim
(See also \ref PRINT)

The following input tells plumed to calculate all the distances between the atoms in GROUPA and the atoms in GROUPB.
In other words the distances between atoms 1 and 2 and the distance between atoms 1 and 3.  The number of distances
more than 0.1 is then printed to a file.
\verbatim
DISTANCES GROUPA=1 GROUPB=2,3 MORE_THAN={RATIONAL R_0=0.1}
PRINT ARG=d1.gt0.1 
\endverbatim
(See also \ref PRINT \ref switchingfunction)
*/
//+ENDPLUMEDOC

class XDistances : public MultiColvar {
private:
  unsigned myc;
public:
  static void registerKeywords( Keywords& keys );
  XDistances(const ActionOptions&);
// active methods:
  virtual double compute();
/// Returns the number of coordinates of the field
  bool isPeriodic(){ return false; }
  Vector getCentralAtom();
};

PLUMED_REGISTER_ACTION(XDistances,"XDISTANCES")
PLUMED_REGISTER_ACTION(XDistances,"YDISTANCES")
PLUMED_REGISTER_ACTION(XDistances,"ZDISTANCES")

void XDistances::registerKeywords( Keywords& keys ){
  MultiColvar::registerKeywords( keys );
  keys.use("ATOMS");  // keys.use("MAX");
  keys.use("MEAN"); keys.use("MIN"); keys.use("LESS_THAN"); keys.use("DHENERGY");
  keys.use("MORE_THAN"); keys.use("BETWEEN"); keys.use("HISTOGRAM"); keys.use("MOMENTS");
  keys.add("atoms-1","GROUP","Calculate the distance between each distinct pair of atoms in the group");
  keys.add("atoms-2","GROUPA","Calculate the distances between all the atoms in GROUPA and all "
                              "the atoms in GROUPB. This must be used in conjuction with GROUPB.");
  keys.add("atoms-2","GROUPB","Calculate the distances between all the atoms in GROUPA and all the atoms "
                              "in GROUPB. This must be used in conjuction with GROUPA.");
}

XDistances::XDistances(const ActionOptions&ao):
PLUMED_MULTICOLVAR_INIT(ao)
{
  if( getName().find("X")!=std::string::npos) myc=0;
  else if( getName().find("Y")!=std::string::npos) myc=1;
  else if( getName().find("Z")!=std::string::npos) myc=2;
  else plumed_error();

  // Read in the atoms
  int natoms=2; readAtoms( natoms );
  // And check everything has been read in correctly
  checkRead();
}

double XDistances::compute(){
   Vector distance; 
   distance=getSeparation( getPosition(0), getPosition(1) );
   const double value=distance[myc];

   Vector myvec; myvec.zero(); 
   // And finish the calculation
   myvec[myc]=+1; addAtomsDerivatives( 1,myvec  );
   myvec[myc]=-1; addAtomsDerivatives( 0,myvec );
   addBoxDerivatives( Tensor(distance,myvec) );
   return value;
}

Vector XDistances::getCentralAtom(){
   addCentralAtomDerivatives( 0, 0.5*Tensor::identity() );
   addCentralAtomDerivatives( 1, 0.5*Tensor::identity() );
   return 0.5*( getPosition(0) + getPosition(1) );
}

}
}

