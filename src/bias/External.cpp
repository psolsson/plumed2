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
#include "Bias.h"
#include "ActionRegister.h"
#include "tools/Grid.h"
#include "tools/Exception.h"
#include "tools/File.h"


using namespace std;


namespace PLMD{
namespace bias{

//+PLUMEDOC BIAS EXTERNAL 
/*
Calculate a restraint that is defined on a grid that is read during start up

\par Examples
The following is an input for a calculation with an external potential that is
defined in the file bias.dat and that acts on the distance between atoms 3 and 5.
\verbatim
DISTANCE ATOMS=3,5 LABEL=d1
EXTERNAL ARG=d1 FILENAME=bias.dat LABEL=external 
\endverbatim
(See also \ref DISTANCE \ref PRINT).

The header in the file bias.dat should read:
\verbatim
#! FIELDS d1 external.bias der_d1
#! SET min_d1 0.0
#! SET max_d1 1.0
#! SET nbins_d1 100
#! SET periodic_d1 false
\endverbatim

This should then be followed by the value of the potential and its derivative
at 100 equally spaced points along the distance between 0 and 1. If you run
with NOSPLINE you do not need to provide derivative information.    

You can also include grids that are a function of more than one collective 
variable.  For instance the following would be the input for an external
potential acting on two torsional angles:
\verbatim
TORSION ATOMS=4,5,6,7 LABEL=t1
TORSION ATOMS=6,7,8,9 LABEL=t2
EXTERNAL ARG=t1,t2 FILENAME=bias.dat LABEL=ext
\endverbatim

The header in the file bias.dat for this calculation would read:
\verbatim
#! FIELDS t1 t2 ext.bias der_t1 der_t2
#! SET min_t1 -pi
#! SET max_t1 +pi
#! SET nbins_t1 100
#! SET periodic_t1 true
#! SET min_t2 -pi
#! SET max_t2 +pi
#! SET nbins_t2 100
#! SET periodic_t2 true
\endverbatim

This would be then followed by 100 blocks of data.  In the first block of data the
value of t1 (the value in the first column) is kept fixed and the value of 
the function is given at 100 equally spaced values for t2 between \f$-pi\f$ and \f$+pi\f$.  In the
second block of data t1 is fixed at \f$-pi + \frac{2pi}{100}\f$ and the value of the function is
given at 100 equally spaced values for t2 between \f$-pi\f$ and \f$+pi\f$. In the third block of
data the same is done but t1 is fixed at \f$-pi + \frac{4pi}{100}\f$ and so on untill you get to 
the 100th block of data where t1 is fixed at \f$+pi\f$. 

Please note the order that the order of arguments in the plumed.dat file must be the same as
the order of arguments in the header of the grid file.
*/
//+ENDPLUMEDOC

class External : public Bias{

private:
  Grid* BiasGrid_;
  
public:
  External(const ActionOptions&);
  ~External();
  void calculate();
  static void registerKeywords(Keywords& keys);
};

PLUMED_REGISTER_ACTION(External,"EXTERNAL")

void External::registerKeywords(Keywords& keys){
  Bias::registerKeywords(keys);
  keys.use("ARG");
  keys.add("compulsory","FILE","the name of the file containing the external potential.");
  keys.addFlag("NOSPLINE",false,"specifies that no spline interpolation is to be used when calculating the energy and forces due to the external potential");
  keys.addFlag("SPARSE",false,"specifies that the external potential uses a sparse grid");
  componentsAreNotOptional(keys);
  keys.addOutputComponent("bias","default","the instantaneous value of the bias potential");
}

External::~External(){
  delete BiasGrid_;
}

External::External(const ActionOptions& ao):
PLUMED_BIAS_INIT(ao),
BiasGrid_(NULL)
{
  string filename;
  parse("FILE",filename);
  if( filename.length()==0 ) error("No external potential file was specified"); 
  bool sparsegrid=false;
  parseFlag("SPARSE",sparsegrid);
  bool nospline=false;
  parseFlag("NOSPLINE",nospline);
  bool spline=!nospline;

  checkRead();

  log.printf("  External potential from file %s\n",filename.c_str());
  if(spline){log.printf("  External potential uses spline interpolation\n");}
  if(sparsegrid){log.printf("  External potential uses sparse grid\n");}
  
  addComponent("bias"); componentIsNotPeriodic("bias");

// read grid
  IFile gridfile; gridfile.open(filename);
  std::string funcl=getLabel() + ".bias";  
  BiasGrid_=Grid::create(funcl,getArguments(),gridfile,sparsegrid,spline,true);
  gridfile.close();
  if(BiasGrid_->getDimension()!=getNumberOfArguments()) error("mismatch between dimensionality of input grid and number of arguments");
  for(unsigned i=0;i<getNumberOfArguments();++i){
    if( getPntrToArgument(i)->isPeriodic()!=BiasGrid_->getIsPeriodic()[i] ) error("periodicity mismatch between arguments and input bias"); 
  } 
}

void External::calculate()
{
  unsigned ncv=getNumberOfArguments();
  vector<double> cv(ncv), der(ncv);

  for(unsigned i=0;i<ncv;++i){cv[i]=getArgument(i);}

  double ene=BiasGrid_->getValueAndDerivatives(cv,der);

  getPntrToComponent("bias")->set(ene);

// set Forces 
  for(unsigned i=0;i<ncv;++i){
   const double f=-der[i];
   setOutputForce(i,f);
  }
}

}
}
