#ifndef __REGRESSIONDATA_IMP_HPP__
#define __REGRESSIONDATA_IMP_HPP__

#include <numeric>

RegressionData::RegressionData(std::vector<Point> & locations, VectorXr & observations, UInt order, MatrixXr & covariates,
	MatrixXi & incidenceMatrix, std::vector<UInt> & bc_indices, std::vector<Real> & bc_values):
		locations_(locations), observations_(observations), covariates_(covariates), incidenceMatrix_(incidenceMatrix),
		order_(order), bc_values_(bc_values), bc_indices_(bc_indices)
{
	nRegions_ = incidenceMatrix_.rows();
	if(locations_.size()==0 && nRegions_==0)
	{
		locations_by_nodes_ = true;

		observations_indices_.resize(observations_.size());
		std::iota(observations_indices_.begin(), observations_indices_.end(), 0);
	}
	else
	{
		locations_by_nodes_ = false;
	}
}

RegressionDataElliptic::RegressionDataElliptic(std::vector<Point> & locations, VectorXr & observations, UInt order,
	Eigen::Matrix<Real,2,2> & K, Eigen::Matrix<Real,2,1> & beta, Real c, MatrixXr & covariates, MatrixXi & incidenceMatrix,
	std::vector<UInt> & bc_indices, std::vector<Real> & bc_values):
		 RegressionData(locations, observations, order, covariates, incidenceMatrix, bc_indices, bc_values), K_(K), beta_(beta), c_(c)
{;}

RegressionDataEllipticSpaceVarying::RegressionDataEllipticSpaceVarying(std::vector<Point> & locations, VectorXr& observations, UInt order,
	const std::vector<Eigen::Matrix<Real,2,2>, Eigen::aligned_allocator<Eigen::Matrix<Real,2,2>>> & K,
	const std::vector<Eigen::Matrix<Real,2,1>, Eigen::aligned_allocator<Eigen::Matrix<Real,2,1>>> & beta,
	const std::vector<Real> & c, const std::vector<Real> & u,
	MatrixXr & covariates, MatrixXi & incidenceMatrix, std::vector<UInt> & bc_indices, std::vector<Real> & bc_values):
		RegressionData(locations, observations, order, covariates, incidenceMatrix, bc_indices, bc_values), K_(K), beta_(beta), c_(c), u_(u)
{;}


#ifdef R_VERSION_
RegressionData::RegressionData(SEXP Rlocations, SEXP Robservations, SEXP Rorder, SEXP Rcovariates, SEXP RincidenceMatrix, SEXP RBCIndices, SEXP RBCValues)
{
	setLocations(Rlocations);
	setIncidenceMatrix(RincidenceMatrix);
	setObservations(Robservations);
	setCovariates(Rcovariates);

	order_ =  INTEGER(Rorder)[0];

	UInt length_indexes = Rf_length(RBCIndices);
	bc_indices_.assign(INTEGER(RBCIndices), INTEGER(RBCIndices) +  length_indexes);
	bc_values_.assign(REAL(RBCValues),REAL(RBCValues) + Rf_length(RBCIndices));
}

RegressionDataElliptic::RegressionDataElliptic(SEXP Rlocations, SEXP Robservations, SEXP Rorder, SEXP RK, SEXP Rbeta,
	SEXP Rc, SEXP Rcovariates, SEXP RincidenceMatrix, SEXP RBCIndices, SEXP RBCValues):
	RegressionData(Rlocations, Robservations, Rorder, Rcovariates, RincidenceMatrix, RBCIndices, RBCValues)
{
	K_.resize(2, 2);
	for(auto i=0; i<2; ++i)
	{
		for(auto j=0; j<2 ; ++j)
		{
			K_(i,j) = REAL(RK)[i+ 2*j];
		}
	}

	beta_.resize(2);
	for(auto i=0; i<2 ; ++i)
	{
		beta_(i) = REAL(Rbeta)[i];
	}

	c_ =  REAL(Rc)[0];
}

RegressionDataEllipticSpaceVarying::RegressionDataEllipticSpaceVarying(SEXP Rlocations, SEXP Robservations, SEXP Rorder, SEXP RK, SEXP Rbeta,
	SEXP Rc, SEXP Ru, SEXP Rcovariates, SEXP RincidenceMatrix, SEXP RBCIndices, SEXP RBCValues):
	RegressionData(Rlocations, Robservations, Rorder, Rcovariates, RincidenceMatrix, RBCIndices, RBCValues), K_(RK), beta_(Rbeta), c_(Rc), u_(Ru)
{;}

void RegressionDataEllipticSpaceVarying::print(std::ostream & out) const
{
	for (auto i=0; i<18; i++)
		out << K_(i);
	for (auto i=0; i<18; i++)
		out << beta_(i);
	for (auto i=0; i<18; i++)
		out << c_(i);
}

void RegressionData::setObservations(SEXP Robservations)
{
	UInt n_obs_ = Rf_length(Robservations);
	observations_.resize(n_obs_);
	observations_indices_.reserve(n_obs_);

	UInt count = 0;
	if(locations_.size() == 0 && nRegions_ == 0)
	{
		locations_by_nodes_ = true;
		for(auto i=0; i<n_obs_; ++i)
		{
			if(!ISNA(REAL(Robservations)[i]))
			{
				observations_[count] = REAL(Robservations)[i];
				count++;
				observations_indices_.push_back(i);
			}
		}
		observations_.conservativeResize(count, Eigen::NoChange);
	}
	else // (locations_.size()>0) NOR (nRegions_>0)
	{
		locations_by_nodes_ = false;
		for(auto i=0; i<n_obs_; ++i)
		{
			observations_[i] = REAL(Robservations)[i];
		}
	}
}

void RegressionData::setCovariates(SEXP Rcovariates)
{
	n_ = INTEGER(Rf_getAttrib(Rcovariates, R_DimSymbol))[0];
	p_ = INTEGER(Rf_getAttrib(Rcovariates, R_DimSymbol))[1];

	covariates_.resize(n_, p_);

	for(auto i=0; i<n_; ++i)
	{
		for(auto j=0; j<p_ ; ++j)
		{
			covariates_(i,j) = REAL(Rcovariates)[i + n_*j];
		}
	}
}

void RegressionData::setLocations(SEXP Rlocations)
{
	n_ = INTEGER(Rf_getAttrib(Rlocations, R_DimSymbol))[0];

	if(n_>0)
	{
		int ndim = INTEGER(Rf_getAttrib(Rlocations, R_DimSymbol))[1];

		if (ndim == 2)
		{
			for(auto i=0; i<n_; ++i)
			{
				locations_.emplace_back(REAL(Rlocations)[i + n_*0], REAL(Rlocations)[i + n_*1]);
			}
		}
		else
		{ //ndim == 3
			for(auto i=0; i<n_; ++i)
			{
				locations_.emplace_back(REAL(Rlocations)[i + n_*0], REAL(Rlocations)[i + n_*1], REAL(Rlocations)[i + n_*2]);
			}
		}
	}
}

void RegressionData::setIncidenceMatrix(SEXP RincidenceMatrix)
{
	nRegions_ = INTEGER(Rf_getAttrib(RincidenceMatrix, R_DimSymbol))[0];
	UInt p = INTEGER(Rf_getAttrib(RincidenceMatrix, R_DimSymbol))[1];

	incidenceMatrix_.resize(nRegions_, p);

	for(auto i=0; i<nRegions_; ++i)
	{
		for(auto j=0; j<p; ++j)
		{
			incidenceMatrix_(i,j) = INTEGER(RincidenceMatrix)[i + nRegions_*j];
		}
	}
}

#endif

void RegressionData::printObservations(std::ostream & out) const
{
	for(auto i=0; i<observations_.size(); i++)
	{
		out << i << "\t" << observations_(i) << std::endl;
	}
}

void RegressionData::printCovariates(std::ostream & out) const
{
	for(auto i=0; i<covariates_.rows(); i++)
	{
		for(auto j=0; j<covariates_.cols(); j++)
		{
			out << covariates_(i,j) << "\t";
		}
		out << std::endl;
	}
}

void RegressionData::printLocations(std::ostream & out) const
{

	for(std::vector<Point>::size_type i=0;i<locations_.size(); i++)
	{
		locations_[i].print(out);
	}
}

void RegressionData::printIncidenceMatrix(std::ostream & out) const
{
	for (auto i=0; i<incidenceMatrix_.rows(); i++)
	{
		for (auto j=0; j<incidenceMatrix_.cols(); j++)
		{
			out << incidenceMatrix_(i,j) << "\t";
		}
		out << std::endl;
	}
}

#endif
