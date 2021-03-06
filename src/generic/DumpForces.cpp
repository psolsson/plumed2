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
#include "core/ActionPilot.h"
#include "core/ActionWithArguments.h"
#include "core/ActionRegister.h"
#include "tools/File.h"

using namespace std;

namespace PLMD{
namespace generic{

//+PLUMEDOC ANALYSIS DUMPFORCES
/*
Dump the force acting on one of a values in a file.  

For a CV this command will dump
the force on the CV itself. Be aware that in order to have the forces on the atoms
you should multiply the output from this argument by the output from DUMPDERIVATIVES.
Furthermore, also note that you can output the forces on multiple quantities simultaneously
by specifying more than one argument. You can control the buffering of output using the \ref FLUSH keyword.


\par Examples
The following input instructs plumed to write a file called forces that contains
the force acting on the distance between atoms 1 and 2. 
\verbatim
DISTANCE ATOM=1,2 LABEL=distance
DUMPFORCES ARG=distance STRIDE=1 FILE=forces
\endverbatim

(See also \ref DISTANCE)

*/
//+ENDPLUMEDOC

class DumpForces :
public ActionPilot,
public ActionWithArguments
{
  string file;
  OFile of;
public:
  void calculate(){}
  DumpForces(const ActionOptions&);
  static void registerKeywords(Keywords& keys);
  void apply(){}
  void update();
  ~DumpForces();
};

PLUMED_REGISTER_ACTION(DumpForces,"DUMPFORCES")

void DumpForces::registerKeywords(Keywords& keys){
  Action::registerKeywords(keys);
  ActionPilot::registerKeywords(keys);
  ActionWithArguments::registerKeywords(keys);
  keys.use("ARG");
  keys.add("compulsory","STRIDE","1","the frequency with which the forces should be output");
  keys.add("compulsory","FILE","the name of the file on which to output the forces");
}

DumpForces::DumpForces(const ActionOptions&ao):
Action(ao),
ActionPilot(ao),
ActionWithArguments(ao)
{
  parse("FILE",file);
  if( file.length()==0 ) error("name of file was not specified");
  of.link(*this);
  of.open(file);
  log.printf("  on file %s\n",file.c_str());
  if( getNumberOfArguments()==0 ) error("no arguments have been specified");
  checkRead();
}


void DumpForces::update(){
  of.fmtField(" %f");
  of.printField("time",getTime());
  for(unsigned i=0;i<getNumberOfArguments();i++){
    of.fmtField(" %15.10f").printField(getPntrToArgument(i)->getName(),getPntrToArgument(i)->getForce());
  }
  of.printField();
}

DumpForces::~DumpForces(){
}

}


}
