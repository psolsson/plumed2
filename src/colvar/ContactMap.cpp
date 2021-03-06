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
#include "Colvar.h"
#include "tools/NeighborList.h"
#include "ActionRegister.h"
#include "tools/SwitchingFunction.h"

#include <string>
#include <cmath>

using namespace std;

namespace PLMD {
namespace colvar {

//+PLUMEDOC COLVAR CONTACTMAP
/*
Calculate the distances between a number of pairs of atoms and transform each distance by a switching function.
The transformed distance can be compared with a set of reference values in order to calculate the squared distance
between two contact maps.

\par Examples

The following example calculates switching functions based on the distances between atoms
1 and 2, 3 and 4 and 4 and 5. The values of these three switching functions are then output
to a file named colvar.

\verbatim
CONTACTMAP ATOMS1=1,2 ATOMS2=3,4 ATOMS3=4,5 ATOMS4=5,6 SWITCH=(RATIONAL R_0=1.5) LABEL=f1
PRINT ARG=f1.* FILE=colvar
\endverbatim

The following example calculates the difference of the current contact map with respect
to a reference provided. 

\verbatim
CONTACTMAP ...
ATOMS1=1,2 REFERENCE1=0.1 
ATOMS2=3,4 REFERENCE2=0.5
ATOMS3=4,5 REFERENCE3=0.25
ATOMS4=5,6 REFERENCE4=0.0
SWITCH=(RATIONAL R_0=1.5) 
LABEL=cmap
CMDIST
... CONTACTMAP

PRINT ARG=cmap FILE=colvar
\endverbatim
(See also \ref PRINT)

*/
//+ENDPLUMEDOC

class ContactMap : public Colvar {   
private:
  bool pbc, dosum, docmdist;
  NeighborList *nl;
  std::vector<SwitchingFunction> sfs;
  vector<double> reference;
public:
  static void registerKeywords( Keywords& keys );
  ContactMap(const ActionOptions&);
  ~ContactMap();
// active methods:
  virtual void calculate();
  void checkFieldsAllowed(){}
};

PLUMED_REGISTER_ACTION(ContactMap,"CONTACTMAP")

void ContactMap::registerKeywords( Keywords& keys ){
  Colvar::registerKeywords( keys );
  keys.add("numbered","ATOMS","the atoms involved in each of the contacts you wish to calculate. "
                   "Keywords like ATOMS1, ATOMS2, ATOMS3,... should be listed and one contact will be "
                   "calculated for each ATOM keyword you specify.");
  keys.reset_style("ATOMS","atoms");
  keys.add("numbered","SWITCH","The switching functions to use for each of the contacts in your map. "
                               "You can either specify a global switching function using SWITCH or one "
                               "switching function for each contact. Details of the various switching "
                               "functions you can use are provided on \\ref switchingfunction."); 
  keys.add("numbered","REFERENCE","A reference value for a given contact, by default is 0.0 "
                               "You can either specify a global reference value using REFERENCE or one "
                               "reference value for each contact."); 
  keys.reset_style("SWITCH","compulsory"); 
  keys.addFlag("SUM",false,"calculate the sum of all the contacts in the input");
  keys.addFlag("CMDIST",false,"calculate the distance with respect to the provided reference contant map");
  keys.addOutputComponent("contact_","default","By not using SUM or CMDIST each contact will be stored in a component");
}

ContactMap::ContactMap(const ActionOptions&ao):
PLUMED_COLVAR_INIT(ao),
pbc(true),
dosum(false),
docmdist(false)
{
  parseFlag("SUM",dosum);
  parseFlag("CMDIST",docmdist);
  if(docmdist==true&&dosum==true) error("You cannot use SUM and CMDIST together");
  bool nopbc=!pbc;
  parseFlag("NOPBC",nopbc);
  pbc=!nopbc;  

  // Read in the atoms
  std::vector<AtomNumber> t, ga_lista, gb_lista;
  for(int i=1;;++i ){
     parseAtomList("ATOMS", i, t );
     if( t.empty() ) break;

     if( t.size()!=2 ){
         std::string ss; Tools::convert(i,ss);
         error("ATOMS" + ss + " keyword has the wrong number of atoms");
     }
     ga_lista.push_back(t[0]); gb_lista.push_back(t[1]);
     t.resize(0); 

     // Add a value for this contact
     std::string num; Tools::convert(i,num);
     if(!dosum&&!docmdist) {addComponentWithDerivatives("contact_"+num); componentIsNotPeriodic("contact_"+num);}
  }
  // Create neighbour lists
  nl= new NeighborList(ga_lista,gb_lista,true,pbc,getPbc());

  // Read in switching functions
  std::string errors; sfs.resize( ga_lista.size() ); unsigned nswitch=0;
  for(unsigned i=0;i<ga_lista.size();++i){
      std::string num, sw1; Tools::convert(i+1, num);
      if( !parseNumbered( "SWITCH", i+1, sw1 ) ) break;
      nswitch++; sfs[i].set(sw1,errors);
      if( errors.length()!=0 ) error("problem reading SWITCH" + num + " keyword : " + errors );
  }
  if( nswitch==0 ){
     std::string sw; parse("SWITCH",sw);
     if(sw.length()==0) error("no switching function specified use SWITCH keyword");
     for(unsigned i=0;i<ga_lista.size();++i){
        sfs[i].set(sw,errors);
        if( errors.length()!=0 ) error("problem reading SWITCH keyword : " + errors );
     }
  } else if( nswitch!=sfs.size()  ){
     std::string num; Tools::convert(nswitch+1, num);
     error("missing SWITCH" + num + " keyword");
  }
  // Read in reference values 
  nswitch=0;
  reference.resize( ga_lista.size() );
  for(unsigned i=0;i<ga_lista.size();++i) reference[i]=0.;
  for(unsigned i=0;i<ga_lista.size();++i){
      if( !parseNumbered( "REFERENCE", i+1, reference[i] ) ) break;
      nswitch++; 
  }
  if( nswitch==0 ){
     parse("REFERENCE",reference[0]);
     for(unsigned i=1;i<ga_lista.size();++i){
       reference[i]=reference[0];
     }
     nswitch = ga_lista.size();
  }
  if ( nswitch != ga_lista.size() ) error("missing REFERENCE keyword");

  // Ouput details of all contacts 
  for(unsigned i=0;i<sfs.size();++i){
     log.printf("  The %dth contact is calculated from atoms : %d %d. Inflection point of switching function is at %s. Reference contact value is %lf\n", i+1, ga_lista[i].serial(), gb_lista[i].serial() , ( sfs[i].description() ).c_str(), reference[i] );
  }

  // Set up if it is just a list of contacts
  if(dosum){
     addValueWithDerivatives(); setNotPeriodic();
     log.printf("  colvar is sum of all contacts in contact map\n");
  }
  if(docmdist){
     addValueWithDerivatives(); setNotPeriodic();
     log.printf("  colvar is distance between the contact map matrix and the provided reference matrix\n");
  }
  requestAtoms(nl->getFullAtomList());
  checkRead();
}

ContactMap::~ContactMap(){
  delete nl;
}

void ContactMap::calculate(){ 
     
 double ncoord=0., coord;
 Tensor virial;
 std::vector<Vector> deriv(getNumberOfAtoms());

   for(unsigned int i=0;i<nl->size();i++) {                   // sum over close pairs
      Vector distance;
      unsigned i0=nl->getClosePair(i).first;
      unsigned i1=nl->getClosePair(i).second;
      if(pbc){
       distance=pbcDistance(getPosition(i0),getPosition(i1));
      } else {
       distance=delta(getPosition(i0),getPosition(i1));
      }

      double dfunc=0.;
      coord = sfs[i].calculateSqr(distance.modulo2(), dfunc) - reference[i];
      if( dosum ) {
         deriv[i0] = deriv[i0] + (-dfunc)*distance ;
         deriv[i1] = deriv[i1] + dfunc*distance ;
         virial=virial+(-dfunc)*Tensor(distance,distance);
         ncoord += coord;
      } else if ( docmdist ) {
         deriv[i0] = deriv[i0] + 2.*coord*(-dfunc)*distance ;
         deriv[i1] = deriv[i1] + 2.*coord*dfunc*distance ;
         virial=virial+2.*coord*(-dfunc)*Tensor(distance,distance);
         ncoord += coord*coord;
      } else {
         Value* val=getPntrToComponent( i );
         setAtomsDerivatives( val, i0, (-dfunc)*distance );
         setAtomsDerivatives( val, i1, dfunc*distance ); 
         setBoxDerivatives( val, (-dfunc)*Tensor(distance,distance) );
         val->set(coord);
      }
   }

  if( dosum || docmdist){
    for(unsigned i=0;i<deriv.size();++i) setAtomsDerivatives(i,deriv[i]);
    setValue           (ncoord);
    setBoxDerivatives  (virial);
  }
}

}
}
