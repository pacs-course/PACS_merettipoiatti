#ifndef __MIXEDFEFPCAFACTORY_H__
#define __MIXEDFEFPCAFACTORY_H__

#include "../../FdaPDE.h"
#include "../../Global_Utilities/Headers/Make_Unique.h"
#include "../../FE_Assemblers_Solvers/Headers/Finite_Element.h"
#include "../../FE_Assemblers_Solvers/Headers/Matrix_Assembler.h"
#include "../../Mesh/Headers/Mesh.h"
#include "../../FE_Assemblers_Solvers/Headers/Param_Functors.h"
#include "../../FE_Assemblers_Solvers/Headers/Solver.h"
#include "FPCAData.h"
#include "FPCAObject.h"
#include "MixedFEFPCA.h"

//! A Factory class: A class for the choice of the cross-validation method to use for the selection of the parameter lambda for each PC.
template<typename Integrator, UInt ORDER, UInt mydim, UInt ndim>
class MixedFEFPCAfactory
{
	public:
	//! A method that takes as parameter a string and builds a pointer to the right object for the cross-validation
	static std::unique_ptr<MixedFEFPCABase<Integrator, ORDER,  mydim,  ndim>> createFPCAsolver(const std::string &validation, const MeshHandler<ORDER,mydim,ndim>& mesh, const FPCAData& fpcaData){

	if(validation=="GCV")
	    return make_unique<MixedFEFPCAGCV<Integrator, ORDER,  mydim, ndim>>(mesh,fpcaData);

	else if(validation=="KFold")
	    return make_unique<MixedFEFPCAKFold<Integrator, ORDER,  mydim, ndim>>(mesh,fpcaData);

	else if(validation=="NoValidation")
	    return make_unique<MixedFEFPCA<Integrator, ORDER,  mydim, ndim>>(mesh,fpcaData);

	else{
		Rprintf("Unknown validation option - using no validation");

		return make_unique<MixedFEFPCA<Integrator, ORDER,  mydim, ndim>>(mesh,fpcaData);
	}

	}

};

#endif