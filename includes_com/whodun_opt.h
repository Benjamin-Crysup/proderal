#ifndef WHODUN_OPT_H
#define WHODUN_OPT_H 1

#include <vector>
#include <stdint.h>
#include <iostream>

/**A function.*/
class ObjectiveFunction{
public:
	/**Allow subclasses.*/
	virtual ~ObjectiveFunction();
	/**
	 * Get the value of the objective function.
	 * @param atValue THe value to evaluate at.
	 * @return The objective value.
	 */
	virtual double getObjective(double atValue) = 0;
};

/**
 * This will do a golden ratio minimization.
 * @param toSearch The objective function to try and minimize.
 * @param lowBound The low value to consider.
 * @param higBound The high value to consider.
 * @param tol The absolute tolerance in the parameter to use for searching.
 * @return The location of the minimum.
 */
double goldenRatioSearch(ObjectiveFunction* toSearch, double lowBound, double higBound, double tol);

/**A multivariate function.*/
class MultivariateObjectiveFunction{
public:
	/**Set up the defaults.*/
	MultivariateObjectiveFunction();
	/**Allow subclasses.*/
	virtual ~MultivariateObjectiveFunction();
	/**The dimension this function operates in.*/
	uintptr_t dimension;
	/**
	 * Get the value.
	 * @param value The location to get at.
	 */
	virtual double getObjective(double* atValue) = 0;
	
	/**The default nudge to use when calculating the gradient.*/
	double defGradDX;
	
	/**
	 * Get the gradient.
	 * @param atValue The location to get at.
	 * @param gradStore The place to put the gradient.
	 */
	virtual void getGradient(double* atValue, double* gradStore);
};

/**Optimization method for a multivariate function.*/
class MultivariateOptimizer{
public:
	/**The place to write log data.*/
	std::ostream* logFile;
	
	/**Allow subclasses.*/
	virtual ~MultivariateOptimizer();
	
	/**
	 * Try to minimize a function.
	 * @param toSearch The thing to minimize.
	 * @param fromLoc On call, the location to start searching from. On return, the optimal.
	 * @param tol The absolute tolerance in the parameters to use for searching (l2 norm).
	 */
	virtual void optimize(MultivariateObjectiveFunction* toSearch, double* fromLoc, double tol) = 0;
};

/**Optimization by gradient descent.*/
class GradientDescentOptimizer : public MultivariateOptimizer{
public:
	/**Basic setup.*/
	GradientDescentOptimizer();
	/**Tear down.*/
	~GradientDescentOptimizer();
	void optimize(MultivariateObjectiveFunction* toSearch, double* fromLoc, double tol);
	/**Storage for a gradient.*/
	std::vector<double> gradStore;
	/**Storage for a location.*/
	std::vector<double> locStore;
};

/**Optimize using L-BFGS.*/
class LBFGSOptimizer : public MultivariateOptimizer{
public:
	/**Basic setup.*/
	LBFGSOptimizer();
	/**Tear down.*/
	~LBFGSOptimizer();
	void optimize(MultivariateObjectiveFunction* toSearch, double* fromLoc, double tol);
	/**The number of steps to save.*/
	uintptr_t maxHist;
	/**Storage for gradient history.*/
	std::vector<double> gradHistS;
	/**Storage for location history.*/
	std::vector<double> paramHistS;
	/**Parameter sized storage.*/
	std::vector<double> qstoreS;
	/**History sized storage.*/
	std::vector<double> astoreS;
	/**The maximum move (double the tolerance this many times). Zero for unlimited.*/
	uintptr_t maxDouble;
	/**If the function itself stops moving, quit early.*/
	double errorTolerance;
};

#endif