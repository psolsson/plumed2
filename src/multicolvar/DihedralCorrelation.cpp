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
#include "tools/Torsion.h"
#include "core/ActionRegister.h"

#include <string>
#include <cmath>

using namespace std;

namespace PLMD{
namespace multicolvar{

//+PLUMEDOC COLVAR DIHCOR
/*
Measures the degree of similarity between dihedral angles.

This colvar calculates the following quantity.

\f[
s = \frac{1}{2} \sum_i \left[ 1 + \cos( \phi_i - \psi_i ) \right]
\f]

where the \f$\phi_i\f$ and \f$\psi\f$ values and the instantaneous values for the \ref TORSION angles of interest.

\par Examples

The following provides an example input for the dihcor action

\verbatim
DIHCOR ...
  ATOMS1=1,2,3,4,5,6,7,8
  ATOMS2=5,6,7,8,9,10,11,12
  LABEL=dih
... DIHCOR
PRINT ARG=dih FILE=colvar STRIDE=10
\endverbatim

In the above input we are calculating the correation between the torsion angle involving atoms 1, 2, 3 and 4 and the torsion angle
involving atoms 5, 6, 7 and 8.	This is then added to the correlation betwene the torsion angle involving atoms 5, 6, 7 and 8 and the
correlation angle involving atoms 9, 10, 11 and 12.

Writing out the atoms involved in all the torsions in this way can be rather tedious. Thankfully if you are working with protein you
can avoid this by using the \ref MOLINFO command.  PLUMED uses the pdb file that you provide to this command to learn
about the topology of the protein molecule.  This means that you can specify torsion angles using the following syntax:

\verbatim
MOLINFO MOLTYPE=protein STRUCTURE=myprotein.pdb
DIHCOR ...
ATOMS1=@phi-3,@psi-3
ATOMS2=@psi-3,@phi-4
ATOMS4=@phi-4,@psi-4
... DIHCOR
PRINT ARG=dih FILE=colvar STRIDE=10
\endverbatim

Here, \@phi-3 tells plumed that you would like to calculate the \f$\phi\f$ angle in the third residue of the protein.
Similarly \@psi-4 tells plumed that you want to calculate the \f$\psi\f$ angle of the 4th residue of the protein.

*/
//+ENDPLUMEDOC

class DihedralCorrelation : public MultiColvar {
private:
public:
  static void registerKeywords( Keywords& keys );
  DihedralCorrelation(const ActionOptions&);
  virtual double compute();
  bool isPeriodic(){ return false; }
  Vector getCentralAtom();
};

PLUMED_REGISTER_ACTION(DihedralCorrelation,"DIHCOR")

void DihedralCorrelation::registerKeywords( Keywords& keys ){
  MultiColvar::registerKeywords( keys );
  keys.use("ATOMS");
}

DihedralCorrelation::DihedralCorrelation(const ActionOptions&ao):
PLUMED_MULTICOLVAR_INIT(ao)
{
  // Read in the atoms
  int natoms=8; readAtoms( natoms );

  // And setup the ActionWithVessel
  if( getNumberOfVessels()==0 ){
     std::string fake_input;
     addVessel( "SUM", fake_input, -1 );  // -1 here means that this value will be named getLabel()
     readVesselKeywords();  // This makes sure resizing is done
  }

  // And check everything has been read in correctly
  checkRead();
}

double DihedralCorrelation::compute(){
  Vector d10,d11,d12;
  d10=getSeparation(getPosition(1),getPosition(0));
  d11=getSeparation(getPosition(2),getPosition(1));
  d12=getSeparation(getPosition(3),getPosition(2));

  Vector dd10,dd11,dd12; PLMD::Torsion t1;
  double phi1  = t1.compute(d10,d11,d12,dd10,dd11,dd12);

  Vector d20,d21,d22;
  d20=getSeparation(getPosition(5),getPosition(4));
  d21=getSeparation(getPosition(6),getPosition(5));
  d22=getSeparation(getPosition(7),getPosition(6));

  Vector dd20,dd21,dd22; PLMD::Torsion t2;
  double phi2 = t2.compute( d20, d21, d22, dd20, dd21, dd22 );

  // Calculate value
  double value = 0.5 * ( 1 + cos( phi2 - phi1 ) );
  // Derivatives wrt phi1
  dd10 *= 0.5*sin( phi2 - phi1 );
  dd11 *= 0.5*sin( phi2 - phi1 );
  dd12 *= 0.5*sin( phi2 - phi1 );
  // And add
  addAtomsDerivatives(0,dd10);
  addAtomsDerivatives(1,dd11-dd10);
  addAtomsDerivatives(2,dd12-dd11);
  addAtomsDerivatives(3,-dd12);
  addBoxDerivatives  (-(extProduct(d10,dd10)+extProduct(d11,dd11)+extProduct(d12,dd12)));
  // Derivative wrt phi2
  dd20 *= -0.5*sin( phi2 - phi1 );
  dd21 *= -0.5*sin( phi2 - phi1 );
  dd22 *= -0.5*sin( phi2 - phi1 );
  // And add
  addAtomsDerivatives(4,dd20);
  addAtomsDerivatives(5,dd21-dd20);
  addAtomsDerivatives(6,dd22-dd21);
  addAtomsDerivatives(7,-dd22);
  addBoxDerivatives  (-(extProduct(d20,dd20)+extProduct(d21,dd21)+extProduct(d22,dd22)));

  return value;
}

Vector DihedralCorrelation::getCentralAtom(){
   addCentralAtomDerivatives( 1, 0.25*Tensor::identity() );
   addCentralAtomDerivatives( 2, 0.25*Tensor::identity() );
   addCentralAtomDerivatives( 5, 0.25*Tensor::identity() );
   addCentralAtomDerivatives( 6, 0.25*Tensor::identity() );
   return 0.25*( getPosition(1) + getPosition(2) + getPosition(5) + getPosition(6) );
}

}
}
