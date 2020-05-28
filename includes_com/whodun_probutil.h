#ifndef WHODUN_PROBUTIL_H
#define WHODUN_PROBUTIL_H 1

#define SUMMATION_QUEUE 50

/**A utility for adding up probabilities.*/
class ProbabilitySummation{
public:
	/**Set up an empty sum.*/
	ProbabilitySummation();
	/**Clear out the queued values.*/
	void updateFromQueue();
	/**
	 * Add a value to the queue.
	 * @param nextLogProb The log10 of the probability to add.
	 */
	void addNextLogProb(double nextLogProb);
	/**
	 * After all values have been managed, get the final value.
	 * @return The log10 of the sum of the probabilities. If P is the maximum probability, calculated as log10(P) + log(sum(10^(log10(p)-log10(P))))
	 */
	double getFinalLogSum();
	/**The next queue end.*/
	int nextQInd;
	/**The running maximum probability.*/
	double maxLogProb;
	/**The running sum of shifted probabiliites.*/
	double runningSum;
	/**Queued values.*/
	double queuedVals[SUMMATION_QUEUE];
};

#endif
